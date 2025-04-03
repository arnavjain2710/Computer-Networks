#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFF_SIZE 1024

using namespace std;

void write_to_log(const string& output , const string& cmd) {
    ofstream logfile("client_log.txt", ios::app);
    if (logfile.is_open()) {
        logfile << "=== Command ===\n";
        logfile << "store " << cmd << "\n\n";
        logfile << "=== Server Response ===\n";
        logfile << output << "\n\n";
        logfile.close();
    } else {
        cerr << "Error opening log file!" << endl;
    }
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFF_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "Socket creation error" << endl;
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        cout << "Invalid address / Address not supported" << endl;
        return -1;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cout << "Connection Failed" << endl;
        return -1;
    }
    
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
        
        bool store_output = false;
        if (cmd.rfind("store ", 0) == 0) { // Check if command starts with "store "
            store_output = true;
            cmd = cmd.substr(6); // Remove "store " from the command
        }
        
        send(sock, cmd.c_str(), cmd.length(), 0);
        
        if (cmd == "exit") {
            cout << "Exiting..." << endl;
            break;
        }
        
        memset(buffer, 0, BUFF_SIZE);
        valread = recv(sock, buffer, BUFF_SIZE - 1, 0);
        if (valread > 0) {
            buffer[valread] = '\0';
            string output(buffer);
            
            if (store_output) {
                write_to_log(output , cmd);
                cout << "Output stored in log file." << endl;
            } else {
                cout << "Output from server:\n" << output << endl;
            }
        }
    }
    
    close(sock);
    return 0;
}
