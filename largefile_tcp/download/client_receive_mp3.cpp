#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 4096

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // 1. Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 2. Connect to server
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return 1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }

    std::cout << "Connected to server. Receiving MP3 file...\n";

    // 3. Open output file in binary mode
    std::ofstream outfile("downloaded_music.mp3", std::ios::binary);
    if (!outfile.is_open()) {
        std::cerr << "Error: Cannot open file for writing.\n";
        close(sock);
        return 1;
    }

    // 4. Receive data
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    long total_bytes = 0;
    while ((bytes_received = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        outfile.write(buffer, bytes_received);
        total_bytes += bytes_received;
    }

    if (bytes_received < 0)
        perror("Receive error");
    else
        std::cout << "File received successfully. Total bytes: " << total_bytes << "\n";

    outfile.close();
    close(sock);

    return 0;
}
