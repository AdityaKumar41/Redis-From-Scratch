#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>

// Function to send a response to the client
void send_response(int client_sock, const std::string &response) {
    send(client_sock, response.c_str(), response.length(), 0);
}

// Function to receive a command from the client
std::string receive_command(int client_sock) {
    char buffer[1024];
    ssize_t len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (len <= 0) {
        return "";
    }
    buffer[len] = '\0';
    return std::string(buffer);
}
