#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;
int main() {
    // Create a UDP socket
    int serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket\n";
        return -1;
    }

    // Bind the socket to an address and port
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY; // Listen on any available interface
    serverAddress.sin_port = htons(8080); // Use port 8080
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error binding socket\n";
        close(serverSocket);
        return -1;
    }

    cout<<"Server is running on port 8080\n";
    char buffer[1024];
    sockaddr_in clientAddress;
    socklen_t clientAddrLen = sizeof(clientAddress);

    int server_timeout = 10;
    // while (true) { // Run forever
    while(server_timeout--){ // run for 10 packets
        // Receive data from the client
        ssize_t bytesRead = recvfrom(serverSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddress, &clientAddrLen);
        if (bytesRead == -1) {
            std::cerr << "Error receiving data\n";
            close(serverSocket);
            return -1;
        }

        // Print received message
        std::cout << "Received ping message: " << buffer << "\n";

        // Send a pong message back to the client
        const char* pongMessage = "pong";
        ssize_t bytesSent = sendto(serverSocket, pongMessage, strlen(pongMessage), 0,
                                    (struct sockaddr*)&clientAddress, clientAddrLen);
        if (bytesSent == -1) {
            std::cerr << "Error sending pong message\n";
            close(serverSocket);
            return -1;
        }
        cout<<"Pong message sent\n";
    }

    // Close the socket
    close(serverSocket);
    return 0;
}
