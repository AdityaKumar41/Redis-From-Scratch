#include <iostream>
#include <unordered_map>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <chrono>
#include <mutex>
#include <thread>

#define SERVER_PORT 1234
#define MAX_CLIENTS 5

// Hash map to store key-value pairs
std::unordered_map<std::string, std::string> keyValueStore;
// Hash map to store expiry times
std::unordered_map<std::string, std::chrono::steady_clock::time_point> expiryStore;

// Mutex for thread-safe access
std::mutex storeMutex;

// Function to clean up expired keys periodically
void cleanup_expired_keys() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::lock_guard<std::mutex> lock(storeMutex);

        auto now = std::chrono::steady_clock::now();
        for (auto it = expiryStore.begin(); it != expiryStore.end();) {
            if (it->second <= now) {
                keyValueStore.erase(it->first);
                it = expiryStore.erase(it);
            } else {
                ++it;
            }
        }
    }
}

// Function to handle the SET command
void handle_set(int client_sock, const std::string &key, const std::string &value) {
    std::lock_guard<std::mutex> lock(storeMutex);
    keyValueStore[key] = value;
    expiryStore.erase(key); // Remove expiry if it exists
    const char *response = "OK";
    send(client_sock, response, strlen(response), 0);
}

// Function to handle the SETEX command
void handle_setex(int client_sock, const std::string &key, int seconds, const std::string &value) {
    std::lock_guard<std::mutex> lock(storeMutex);
    keyValueStore[key] = value;
    // Store the expiry time
    expiryStore[key] = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    const char *response = "OK";
    send(client_sock, response, strlen(response), 0);
}

// Function to handle the GET command
void handle_get(int client_sock, const std::string &key) {
    std::lock_guard<std::mutex> lock(storeMutex);
    if (keyValueStore.find(key) != keyValueStore.end()) {
        // Check if the key has expired
        if (expiryStore.find(key) == expiryStore.end() || expiryStore[key] > std::chrono::steady_clock::now()) {
            std::string value = keyValueStore[key];
            send(client_sock, value.c_str(), value.length(), 0);
        } else {
            // Key has expired
            keyValueStore.erase(key);
            expiryStore.erase(key);
            const char *response = "Key expired";
            send(client_sock, response, strlen(response), 0);
        }
    } else {
        const char *response = "Key not found";
        send(client_sock, response, strlen(response), 0);
    }
}

// Function to handle the DEL command
void handle_del(int client_sock, const std::string &key) {
    std::lock_guard<std::mutex> lock(storeMutex);
    if (keyValueStore.erase(key) > 0) {
        expiryStore.erase(key);
        const char *response = "Key deleted";
        send(client_sock, response, strlen(response), 0);
    } else {
        const char *response = "Key not found";
        send(client_sock, response, strlen(response), 0);
    }
}

// Function to handle the EXISTS command
void handle_exists(int client_sock, const std::string &key) {
    std::lock_guard<std::mutex> lock(storeMutex);
    if (keyValueStore.find(key) != keyValueStore.end() &&
        (expiryStore.find(key) == expiryStore.end() || expiryStore[key] > std::chrono::steady_clock::now())) {
        const char *response = "1";
        send(client_sock, response, strlen(response), 0);
    } else {
        const char *response = "0";
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
    ssize_t len;

    while ((len = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[len] = '\0';  // Null-terminate the received data
        std::string command(buffer);

        if (command.substr(0, 3) == "SET") {
            size_t first_space = command.find(' ', 4);
            if (first_space != std::string::npos) {
                std::string key = command.substr(4, first_space - 4);
                std::string value = command.substr(first_space + 1);
                handle_set(client_sock, key, value);
            }
        } else if (command.substr(0, 5) == "SETEX") {
            size_t first_space = command.find(' ', 6);
            if (first_space != std::string::npos) {
                size_t second_space = command.find(' ', first_space + 1);
                if (second_space != std::string::npos) {
                    std::string key = command.substr(6, first_space - 6);
                    int seconds = std::stoi(command.substr(first_space + 1, second_space - first_space - 1));
                    std::string value = command.substr(second_space + 1);
                    handle_setex(client_sock, key, seconds, value);
                }
            }
        } else if (command.substr(0, 3) == "GET") {
            std::string key = command.substr(4);
            handle_get(client_sock, key);
        } else if (command.substr(0, 3) == "DEL") {
            std::string key = command.substr(4);
            handle_del(client_sock, key);
        } else if (command.substr(0, 6) == "EXISTS") {
            std::string key = command.substr(7);
            handle_exists(client_sock, key);
        } else if (command == "PING") {
            handle_ping(client_sock);
        } else {
            const char *response = "Invalid command";
            send(client_sock, response, strlen(response), 0);
        }
    }

    // When the client disconnects or sends an invalid command, close the socket
    close(client_sock);
    std::cout << "Client disconnected." << std::endl;
}

int main() {
    // Create socket and bind to port
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Failed to create socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to bind socket");
        return 1;
    }

    if (listen(server_sock, MAX_CLIENTS) == -1) {
        perror("Failed to listen");
        return 1;
    }

    std::cout << "Server listening on port " << SERVER_PORT << std::endl;

    std::thread cleanup_thread(cleanup_expired_keys);

    while (true) {
        int client_sock = accept(server_sock, nullptr, nullptr);
        if (client_sock == -1) {
            perror("Failed to accept client");
            continue;
        }

        std::thread(process_client, client_sock).detach();  // Handle client in a separate thread
    }

    close(server_sock);
    return 0;
}
