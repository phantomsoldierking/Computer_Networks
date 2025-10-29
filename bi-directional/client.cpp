#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

void receiveMessages(int socket) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(socket, buffer, sizeof(buffer));
        if (valread <= 0) {
            std::cout << "Server disconnected.\n";
            break;
        }
        std::cout << "Server: " << buffer << std::endl;
    }
}

void sendMessages(int socket) {
    std::string msg;
    while (true) {
        std::getline(std::cin, msg);
        send(socket, msg.c_str(), msg.size(), 0);
    }
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // 1. Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // 2. Connect
    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    std::cout << "Connected to server!\n";

    // 3. Launch threads
    std::thread recvThread(receiveMessages, sock);
    std::thread sendThread(sendMessages, sock);

    recvThread.join();
    sendThread.join();

    close(sock);
    return 0;
}
