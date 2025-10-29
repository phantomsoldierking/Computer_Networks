#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 4096  // 4 KB buffer

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // 1. Create a TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // 2. Bind to a local port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    // 3. Start listening for connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    // 4. Accept a connection from a client
    client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (client_socket < 0) {
        perror("Accept failed");
        close(server_fd);
        return 1;
    }

    std::cout << "Client connected. Receiving MP3 file...\n";

    // 5. Open file for writing in binary mode
    std::ofstream outfile("received_music.mp3", std::ios::binary);
    if (!outfile.is_open()) {
        std::cerr << "Error: Cannot open file for writing.\n";
        close(client_socket);
        close(server_fd);
        return 1;
    }

    // 6. Receive and write file data
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    long total_bytes = 0;

    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        outfile.write(buffer, bytes_received);
        total_bytes += bytes_received;
    }

    if (bytes_received < 0)
        perror("Receive error");
    else
        std::cout << "File received successfully. Total bytes: " << total_bytes << "\n";

    // 7. Clean up
    outfile.close();
    close(client_socket);
    close(server_fd);

    return 0;
}
