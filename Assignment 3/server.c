#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>

#define PORT 12345
#define CHUNK_SIZE 100

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[CHUNK_SIZE];
    int bytes_read, file_fd;
    
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) 
        error("Error opening socket");

    // Initialize server address structure
    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) 
        error("Error on binding");

    // Listen for connections
    printf("Server listening....");
    listen(server_fd, 5);
    client_len = sizeof(client_addr);
    

    while(1)
    {
        // Accept connection from client
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) 
            error("Error on accept");
    
        // Receive filename from client
        memset(buffer, 0, CHUNK_SIZE);
        int n = recv(client_fd, buffer, CHUNK_SIZE-1, 0);
        if (n < 0) 
            error("Error receiving filename");
    
        printf("Received filename: %s\n", buffer);
    
        // Open the file
        file_fd = open(buffer, O_RDONLY);
        if (file_fd < 0) {
            // File not found, send error message to client
            const char *error_message = "ERR 01: File Not Found";
            printf("File not found.\n");
            send(client_fd, error_message, strlen(error_message), 0);
            close(client_fd);
            // close(server_fd);
            // return 0;
        }
    
        // Read the file in chunks and send to the client
        while ((bytes_read = read(file_fd, buffer, CHUNK_SIZE)) > 0) {
            n = send(client_fd, buffer, bytes_read, 0);
            if (n < 0) 
                error("Error sending file");
        }
    
        printf("File transfer completed.\n");
    
        // Close file and client connection
        close(file_fd);
        close(client_fd);
    }

    close(server_fd);

    return 0;
}
