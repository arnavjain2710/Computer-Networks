#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 100

int is_delimiter(char c) {
    return (c == ' ' || c == ',' || c == ';' || c == ':' || c == '.' || c == '\t' || c == '\n' || c == '\r');
}

int main() {
    int sock_fd;
    struct sockaddr_in server_addr;

    // Create socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Connect to server
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Get filename from user
    char filename[BUFFER_SIZE];
    printf("Enter filename: ");
    fgets(filename, BUFFER_SIZE, stdin);
    filename[strcspn(filename, "\n")] = '\0';

    // Send filename to server
    if (send(sock_fd, filename, strlen(filename) + 1, 0) == -1) {
        perror("send");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Receive file and count words/bytes
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    int output_fd = open("received_file.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    size_t total_bytes = 0;
    int in_word = 0, word_count = 0;

    while ((bytes_received = recv(sock_fd, buffer, BUFFER_SIZE, 0)) > 0) {
        total_bytes += bytes_received;
        write(output_fd, buffer, bytes_received);

        for (int i = 0; i < bytes_received; i++) {
            if (is_delimiter(buffer[i])) {
                in_word = 0;
            } else {
                if (!in_word) {
                    word_count++;
                    in_word = 1;
                }
            }
        }
    }

    close(output_fd);
    close(sock_fd);

    if (total_bytes == 0) {
        printf("ERR 01: File Not Found\n");
        remove("received_file.txt");
    } else {
        printf("File transfer successful. Size: %zu bytes, Words: %d\n", total_bytes, word_count);
    }

    return 0;
}
