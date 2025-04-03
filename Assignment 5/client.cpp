#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>

const int BUFFER_SIZE = 1024;
const int PORT = 12345;

void receive_list(int sock) {
    char buffer[BUFFER_SIZE];
    std::string full_response;
    
    while (true) {
        int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) break;
        
        full_response.append(buffer, bytes_received);
        if (full_response.find("END_OF_LIST") != std::string::npos) {
            break;
        }
    }

    size_t end_pos = full_response.find("END_OF_LIST");
    std::cout << "\nAvailable files:\n" 
              << full_response.substr(0, end_pos) << std::endl;
}

void receive_file(int sock, const std::string& filename) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
    
    if (bytes_received <= 0) {
        std::cout << "Error receiving file header" << std::endl;
        return;
    }

    std::string header(buffer, bytes_received);
    if (header.substr(0, 8) != "FILESIZE") {
        std::cout << header;
        return;
    }

    // Generate client copy filename
    size_t dot_pos = filename.find_last_of('.');
    std::string output_name;
    if (dot_pos != std::string::npos) {
        output_name = filename.substr(0, dot_pos) + 
                     "_clientcopy" + 
                     filename.substr(dot_pos);
    } else {
        output_name = filename + "_clientcopy";
    }

    size_t space_pos = header.find(' ');
    size_t newline_pos = header.find('\n');
    long file_size = std::stol(header.substr(space_pos + 1, newline_pos - space_pos - 1));
    
    std::ofstream file(output_name, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Error creating local file" << std::endl;
        return;
    }

    long total_received = 0;
    while (total_received < file_size) {
        bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) break;
        
        // Write received chunk to file
        file.write(buffer, bytes_received);
        total_received += bytes_received;
        
        // Optional: Display chunk info
        std::cout << "Received chunk: " << bytes_received << " bytes" << std::endl;
    }

    file.close();
    std::cout << "Total received: " << total_received << " bytes" << std::endl;
    std::cout << "File saved as: " << output_name << std::endl;
}


int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        return 1;
    }

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return 1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }

    char welcome[BUFFER_SIZE];
    recv(sock, welcome, BUFFER_SIZE, 0);
    std::cout << welcome;

    while (true) {
        std::cout << "\nEnter command (LIST/GET/QUIT): ";
        std::string command;
        std::getline(std::cin, command);

        if (command == "LIST") {
            send(sock, "LIST\n", 5, 0);
            receive_list(sock);
        }
        else if (command.substr(0, 3) == "GET") {
            send(sock, (command + "\n").c_str(), command.size() + 1, 0);
            receive_file(sock, command.substr(4));
        }
        else if (command == "QUIT") {
            send(sock, "QUIT\n", 5, 0);
            break;
        }
        else {
            std::cout << "Invalid command" << std::endl;
        }
    }

    close(sock);
    std::cout << "Connection closed" << std::endl;
    return 0;
}
