#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

// Constants
constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;

/**
 * Helper function to create and bind a UDP socket.
 * @return socket file descriptor on success, -1 on failure.
 */
int createAndBindSocket() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    serverAddr.sin_port = htons(PORT);

    if (bind(sockfd, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int main() {
    int sockfd = createAndBindSocket();
    if (sockfd < 0) return EXIT_FAILURE;

    char buffer[BUFFER_SIZE];
    sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);

    std::cout << "UDP Server listening on port " << PORT << "...\n";

    while (true) {
        // Receive message from client
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                             reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (n < 0) {
            perror("Receive failed");
            continue;
        }
        buffer[n] = '\0'; // Null-terminate string

        std::cout << "Client says: " << buffer << "\n";

        // Send acknowledgment back
        std::string response = "Message received: " + std::string(buffer);
        sendto(sockfd, response.c_str(), response.size(), 0,
               reinterpret_cast<sockaddr*>(&clientAddr), clientLen);
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
