#ifndef HELPER_H
#define HELPER_H

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>

// Function to send a response to the client
inline void send_response(int client_sock, const std::string &response) {
    ssize_t bytes_sent = send(client_sock, response.c_str(), response.length(), 0);
    if (bytes_sent == -1) {
        std::cerr << "Error: Failed to send response to client" << std::endl;
    }
}

// Function to receive a command from the client
inline std::string receive_command(int client_sock) {
    char buffer[1024];
    ssize_t len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (len <= 0) {
        if (len == 0) {
            std::cerr << "Client disconnected" << std::endl;
        } else {
            std::cerr << "Error: Failed to receive data from client" << std::endl;
        }
        return "";
    }
    buffer[len] = '\0'; // Null-terminate the received data
    return std::string(buffer);
}

#endif // HELPER_H
