#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <algorithm>  // needed for std::remove

#define PORT 8080

std::vector<int> clients;   // all connected clients
std::mutex clientsMutex;    // protects the clients list

// Broadcasts message to all connected clients (except sender)
void broadcastMessage(const std::string &message, int senderSocket) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (int clientSocket : clients) {
        if (clientSocket != senderSocket) {
            send(clientSocket, message.c_str(), message.size(), 0);
        }
    }
}

// Handles a single client connection
void handleClient(int clientSocket, std::string clientIP) {
    char buffer[1024];
    std::string joinMsg = clientIP + " joined the chat.\n";

    std::cout << joinMsg;              // Print join message on server
    broadcastMessage(joinMsg, clientSocket); // Notify other clients

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = read(clientSocket, buffer, sizeof(buffer));
        if (bytesRead <= 0) {
            std::string leaveMsg = clientIP + " left the chat.\n";
            std::cout << leaveMsg;
            broadcastMessage(leaveMsg, clientSocket);

            close(clientSocket);
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
            break;
        }

        // Build message: "<client-ip>: message"
        std::string msg = clientIP + ": " + std::string(buffer);

        // ✅ Print every message from every client on server console
        std::cout << msg;

        // Broadcast it to all other clients
        broadcastMessage(msg, clientSocket);
    }
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // 1. Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket failed");
        return 1;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    // 2. Bind to any IP
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }

    // 3. Listen for connections
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        return 1;
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    // 4. Accept clients continuously
    while (true) {
        int new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        std::string clientIP = inet_ntoa(address.sin_addr);
        std::cout << "New client connected: " << clientIP << "\n";

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(new_socket);
        }

        // Create a thread for this client
        std::thread(handleClient, new_socket, clientIP).detach();
    }

    close(server_fd);
    return 0;
}
