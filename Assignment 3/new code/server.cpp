#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 100

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        // Accept client connection
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        // Receive filename from client
        char filename[BUFFER_SIZE] = {0};
        ssize_t bytes_received;
        size_t total_received = 0;
        int filename_received = 0;

        while (total_received < sizeof(filename) - 1) {
            bytes_received = recv(client_fd, filename + total_received, sizeof(filename) - total_received - 1, 0);
            if (bytes_received <= 0) break;
            total_received += bytes_received;
            if (strchr(filename, '\0')) {
                filename_received = 1;
                break;
            }
        }

        if (!filename_received) {
            close(client_fd);
            continue;
        }

        // Open file
        int file_fd = open(filename, O_RDONLY);
        if (file_fd == -1) {
            close(client_fd);
            continue;
        }

        // Read and send file in chunks
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;

        while ((bytes_read = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
            ssize_t bytes_sent = 0;
            while (bytes_sent < bytes_read) {
                ssize_t sent = send(client_fd, buffer + bytes_sent, bytes_read - bytes_sent, 0);
                if (sent == -1) {
                    perror("send");
                    break;
                }
                bytes_sent += sent;
            }
        }

        close(file_fd);
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
