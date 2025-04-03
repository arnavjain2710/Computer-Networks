#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>

const int BUFFER_SIZE = 1024;
const int PORT = 12345;
std::mutex log_mutex;

void log_message(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::ofstream logfile("server.log", std::ios::app);
    if (logfile.is_open()) {
        logfile << message << std::endl;
    }
}

std::vector<std::string> list_files() {
    std::vector<std::string> files;
    DIR *dir;
    struct dirent *ent;
    
    if ((dir = opendir(".")) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_type == DT_REG) { 
                files.push_back(ent->d_name);
            }
        }
        closedir(dir);
    }
    return files;
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    std::string welcome = "Welcome to Simple File Server\n";
    send(client_socket, welcome.c_str(), welcome.size(), 0);

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        
        if (bytes_received <= 0) break;
        
        std::string command(buffer);
        command = command.substr(0, command.find('\n'));
        log_message("Command received: " + command);

        if (command == "LIST") {
            auto files = list_files();
            std::string response;
            for (const auto& file : files) {
                response += file + "\n";
            }
            response += "END_OF_LIST\n";
            send(client_socket, response.c_str(), response.size(), 0);
        }
        else if (command.substr(0, 3) == "GET") {
            std::string filename = command.substr(4);
            struct stat file_stat;
            
            if (stat(filename.c_str(), &file_stat) == -1) {
                std::string error = "ERROR: File Not Found\n";
                send(client_socket, error.c_str(), error.size(), 0);
                continue;
            }

            std::ifstream file(filename, std::ios::binary);
            if (!file.is_open()) {
                std::string error = "ERROR: File Access Denied\n";
                send(client_socket, error.c_str(), error.size(), 0);
                continue;
            }

            std::string header = "FILESIZE " + std::to_string(file_stat.st_size) + "\n";
            send(client_socket, header.c_str(), header.size(), 0);

            char file_buffer[BUFFER_SIZE];
            while (!file.eof()) {
                file.read(file_buffer, BUFFER_SIZE);
                send(client_socket, file_buffer, file.gcount(), 0);
            }
            file.close();
        }
        else if (command == "QUIT") {
            std::string goodbye = "Goodbye!\n";
            send(client_socket, goodbye.c_str(), goodbye.size(), 0);
            break;
        }
        else {
            std::string error = "ERROR: Invalid Command\n";
            send(client_socket, error.c_str(), error.size(), 0);
        }
    }
    close(client_socket);
    log_message("Client disconnected");
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        return 1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        std::thread(handle_client, client_socket).detach();
        log_message("New client connected");
    }

    close(server_fd);
    return 0;
}
