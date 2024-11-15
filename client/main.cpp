#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 1234
#define SERVER_IP "127.0.0.1" // Localhost

// Function to send the command to the server
void send_command(int sock, const char *command) {
    send(sock, command, strlen(command), 0);
    std::cout << "Sending command: " << command << std::endl;
}

// Function to receive response from the server
void receive_response(int sock) {
    char buffer[1024];
    ssize_t len = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (len > 0) {
        buffer[len] = '\0';
        std::cout << "Server Response: " << buffer << std::endl;
    } else {
        std::cerr << "Error receiving response" << std::endl;
    }
}

int main() {
    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    // Set up server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Connection failed" << std::endl;
        return -1;
    }

    // Input loop for command
    std::string command;
    while (true) {
        std::cout << "Enter command (SET key value, GET key, or PING): ";
        std::getline(std::cin, command);

        if (command.substr(0, 3) == "SET") {
            send_command(sock, command.c_str());
            receive_response(sock);
        } else if (command.substr(0, 3) == "GET") {
            send_command(sock, command.c_str());
            receive_response(sock);
        } else if (command == "PING") {
            send_command(sock, "PING");
            receive_response(sock);
        } else {
            std::cout << "Invalid command. Use SET key value, GET key, or PING." << std::endl;
        }
    }

    close(sock);
    return 0;
}
