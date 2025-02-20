// client.cpp
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFF_SIZE 1024

using namespace std;

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFF_SIZE] = {0};

    // Create the TCP socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "Socket creation error" << endl;
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        cout << "Invalid address / Address not supported" << endl;
        return -1;
    }
    
    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cout << "Connection Failed" << endl;
        return -1;
    }
    
    // Receive and display initial server information
    int valread = recv(sock, buffer, BUFF_SIZE - 1, 0);
    if (valread > 0) {
        buffer[valread] = '\0';
        cout << buffer << endl;
    }
    
    while (true) {
        cout << "Enter command (or 'exit' to disconnect): ";
        string cmd;
        getline(cin, cmd);
        if (cmd.empty()) continue;
        
        // Send the command to the server
        send(sock, cmd.c_str(), cmd.length(), 0);
        
        if (cmd == "exit") {
            cout << "Exiting..." << endl;
            break;
        }
        
        // Receive the output from the server
        memset(buffer, 0, BUFF_SIZE);
        valread = recv(sock, buffer, BUFF_SIZE - 1, 0);
        if (valread > 0) {
            buffer[valread] = '\0';
            cout << "Output from server:\n" << buffer << endl;
        }
    }
    
    close(sock);
    return 0;
}

// commands:
// g++ -o client client.cpp
// ./client 