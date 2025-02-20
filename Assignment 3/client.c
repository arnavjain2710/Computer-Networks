#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <ctype.h>
#include <arpa/inet.h>

#define PORT 12345
#define CHUNK_SIZE 100

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int is_word_delimiter(char c) {
    return (c == ' ' || c == '\t' || c == ',' || c == ';' || c == ':' || c == '.' || c == '\n');
}

int main() {
    int sockfd, file_fd;
    struct sockaddr_in server_addr;
    char buffer[CHUNK_SIZE];
    char *filename = NULL;
    int bytes_received, total_bytes = 0, word_count = 0;
    int in_word = 0;
    
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("Error opening socket");

    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Localhost
    server_addr.sin_port = htons(PORT);

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) 
        error("Connection failed");

    // Input filename
    printf("Enter filename to retrieve: ");
    filename = (char *)malloc(100 * sizeof(char));
    if (filename == NULL) 
        error("Memory allocation failed");

    fgets(filename, 100, stdin);
    filename[strcspn(filename, "\n")] = '\0';  // Remove the trailing newline

    // Send filename to server
    if (send(sockfd, filename, strlen(filename), 0) < 0)
        error("Error sending filename");

    // Receive potential error message
    memset(buffer, 0, CHUNK_SIZE);
    bytes_received = recv(sockfd, buffer, CHUNK_SIZE, 0);
    if (bytes_received < 0)
        error("Error receiving response");

    if (strncmp(buffer, "ERR 01: File Not Found", 22) == 0) {
        // Handle file not found error
        printf("Error: File not found on server. Exiting.\n");
        close(sockfd);
        free(filename);
        return 1;
    }

    // Open the new file for writing
    file_fd = open("received_file.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_fd < 0) 
        error("Error opening file for writing");

    // Receive file in chunks
    while (bytes_received) {
        total_bytes += bytes_received;
        
        // Count words
        for (int i = 0; i < bytes_received; i++) {
            if (is_word_delimiter(buffer[i])) {
                if (in_word) {
                    word_count++;
                    in_word = 0;
                }
            } else {
                in_word = 1;
            }
        }

        write(file_fd, buffer, bytes_received);
        bytes_received = recv(sockfd, buffer, CHUNK_SIZE, 0);
    }

    if (in_word) {
        word_count++;
        in_word = 0;
    }

    // Close the file and socket
    close(file_fd);
    close(sockfd);

    // Print the result
    printf("The file transfer is successful. Size of the file = %d bytes, no. of words = %d\n", total_bytes, word_count);

    free(filename);
    return 0;
}
