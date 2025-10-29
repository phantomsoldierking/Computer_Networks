#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 4096

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // 1. Create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // 2. Bind to local port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    // 3. Listen for client
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    std::cout << "Server ready to send MP3 on port " << PORT << "...\n";

    // 4. Accept client connection
    client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (client_socket < 0) {
        perror("Accept failed");
        close(server_fd);
        return 1;
    }

    std::cout << "Client connected. Sending MP3 file...\n";

    // 5. Open MP3 file
    std::ifstream infile("music.mp3", std::ios::binary);
    if (!infile.is_open()) {
        std::cerr << "Error: Cannot open MP3 file.\n";
        close(client_socket);
        close(server_fd);
        return 1;
    }

    // 6. Send data in chunks
    char buffer[BUFFER_SIZE];
    long total_bytes = 0;
    while (!infile.eof()) {
        infile.read(buffer, BUFFER_SIZE);
        std::streamsize bytes_read = infile.gcount();
        if (bytes_read > 0) {
            send(client_socket, buffer, bytes_read, 0);
            total_bytes += bytes_read;
        }
    }

    std::cout << "File sent successfully. Total bytes: " << total_bytes << "\n";

    infile.close();
    close(client_socket);
    close(server_fd);

    return 0;
}
