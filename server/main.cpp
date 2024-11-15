#include <iostream>
#include <unordered_map>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 1234
#define MAX_CLIENTS 5

// Hash map to store key-value pairs (in memory)
std::unordered_map<std::string, std::string> keyValueStore;

// Function to handle the SET command
void handle_set(int client_sock, const std::string &key, const std::string &value) {
    keyValueStore[key] = value;
    const char *response = "OK";
    send(client_sock, response, strlen(response), 0);
}

// Function to handle the GET command
void handle_get(int client_sock, const std::string &key) {
    if (keyValueStore.find(key) != keyValueStore.end()) {
        std::string value = keyValueStore[key];
        send(client_sock, value.c_str(), value.length(), 0);
    } else {
        const char *response = "Key not found";
        send(client_sock, response, strlen(response), 0);
    }
}

// Function to handle the PING command
void handle_ping(int client_sock) {
    const char *response = "PONG";
    send(client_sock, response, strlen(response), 0);
}

// Function to process incoming client commands
void process_client(int client_sock) {
    char buffer[1024];
    ssize_t len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (len > 0) {
        buffer[len] = '\0';
        std::string command(buffer);

        // Parse and process the command
        if (command.substr(0, 3) == "SET") {
            size_t space_pos = command.find(' ', 4);
            if (space_pos != std::string::npos) {
                std::string key = command.substr(4, space_pos - 4);
                std::string value = command.substr(space_pos + 1);
                handle_set(client_sock, key, value);
            }
        } else if (command.substr(0, 3) == "GET") {
            std::string key = command.substr(4);
            handle_get(client_sock, key);
        } else if (command == "PING") {
            handle_ping(client_sock);
        } else {
            const char *response = "Invalid command";
            send(client_sock, response, strlen(response), 0);
        }
    }
}

int main() {
    // Create socket
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    // Set up server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Error binding socket" << std::endl;
        return -1;
    }

    // Listen for incoming connections
    if (listen(server_sock, MAX_CLIENTS) == -1) {
        std::cerr << "Error listening on socket" << std::endl;
        return -1;
    }

    std::cout << "Server is listening on port " << SERVER_PORT << std::endl;

    // Accept incoming client connections and process commands
    while (true) {
        int client_sock = accept(server_sock, nullptr, nullptr);
        if (client_sock == -1) {
            std::cerr << "Error accepting client connection" << std::endl;
            continue;
        }

        std::cout << "Client connected!" << std::endl;

        // Process commands from the client
        process_client(client_sock);

        // Close the client socket
        close(client_sock);
        std::cout << "Client disconnected." << std::endl;
    }

    // Close the server socket
    close(server_sock);
    return 0;
}
