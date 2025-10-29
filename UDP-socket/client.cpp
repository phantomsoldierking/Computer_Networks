#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

// Constants
constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;
constexpr const char* SERVER_IP = "127.0.0.1"; // Change if server is remote

/**
 * Helper function to create a UDP socket.
 * @return socket file descriptor on success, -1 on failure.
 */
int createSocket() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }
    return sockfd;
}

int main() {
    int sockfd = createSocket();
    if (sockfd < 0) return EXIT_FAILURE;

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    char buffer[BUFFER_SIZE];
    std::string message;

    std::cout << "Enter message to send (type 'exit' to quit):\n";

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, message);

        if (message == "exit") break;

        // Send message to server
        sendto(sockfd, message.c_str(), message.size(), 0,
               reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));

        // Receive response
        socklen_t serverLen = sizeof(serverAddr);
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                             reinterpret_cast<sockaddr*>(&serverAddr), &serverLen);
        if (n < 0) {
            perror("Receive failed");
            continue;
        }
        buffer[n] = '\0';

        std::cout << "Server replied: " << buffer << "\n";
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
