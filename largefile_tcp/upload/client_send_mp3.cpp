#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"  // Change to server's IP if remote
#define PORT 8080
#define BUFFER_SIZE 4096

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // 1. Create a TCP socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 2. Convert IP address from text to binary
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return 1;
    }

    // 3. Connect to the server
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }

    std::cout << "Connected to server. Sending MP3 file...\n";

    // 4. Open MP3 file in binary mode
    std::ifstream infile("music.mp3", std::ios::binary);
    if (!infile.is_open()) {
        std::cerr << "Error: Cannot open MP3 file.\n";
        close(sock);
        return 1;
    }

    // 5. Send file data in chunks
    char buffer[BUFFER_SIZE];
    long total_bytes = 0;
    while (!infile.eof()) {
        infile.read(buffer, BUFFER_SIZE);
        std::streamsize bytes_read = infile.gcount();
        if (bytes_read > 0) {
            send(sock, buffer, bytes_read, 0);
            total_bytes += bytes_read;
        }
    }

    std::cout << "File sent successfully. Total bytes: " << total_bytes << "\n";

    // 6. Clean up
    infile.close();
    close(sock);

    return 0;
}
