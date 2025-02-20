#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <ctime>

#define PING_COUNT 10
#define TIMEOUT 1  // Timeout in seconds
using namespace std;
int main() {
    // Create a UDP socket
    int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Error creating socket\n";
        return -1;
    }
    
    cout<<"Client is running\n";
    // Set up the server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server's IP address
    serverAddress.sin_port = htons(8080); // Server's port

    // Set the timeout for receiving data
    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cerr << "Error setting socket timeout\n";
        close(clientSocket);
        return -1;
    }

    char message[] = "ping";
    char buffer[1024];
    sockaddr_in serverReply;
    socklen_t serverAddrLen = sizeof(serverReply);

    double totalRTT = 0;

    for (int i = 0; i < PING_COUNT; ++i) {
        cout<<"Sending ping message "<<i+1<<"\n";
        // Record the current time before sending the message
        clock_t startTime = clock();

        // Send the ping message to the server
        ssize_t bytesSent = sendto(clientSocket, message, strlen(message), 0,
                                    (struct sockaddr*)&serverAddress, sizeof(serverAddress));
        if (bytesSent == -1) {
            std::cerr << "Error sending ping message\n";
            close(clientSocket);
            return -1;
        }

        // Wait for the pong message from the server
        ssize_t bytesReceived = recvfrom(clientSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&serverReply, &serverAddrLen);
        if (bytesReceived == -1) {
            std::cerr << "Request timed out, packet lost\n";
        } else {
            // Record the time after receiving the pong message
            clock_t endTime = clock();

            // Calculate and print the Round Trip Time (RTT)
            double rtt = double(endTime - startTime) / CLOCKS_PER_SEC * 1000.0; // RTT in milliseconds
            totalRTT += rtt;
            std::cout << "Ping " << i + 1 << " RTT: " << rtt << " ms\n";
        }
    }
    cout<<"Average RTT: "<<totalRTT/PING_COUNT<<" ms\n";
    cout<<"Closing client\n";

    // Close the socket
    close(clientSocket);
    return 0;
}
