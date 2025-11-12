#include <iostream>
#include <thread>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

#define PORT 8080

void receiveMessages(int sock) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(sock, buffer, sizeof(buffer));
        if (valread <= 0) {
            std::cout << "Server disconnected.\n";
            break;
        }
        std::cout << "\n" << buffer << std::flush;
        std::cout << "You: " << std::flush;
    }
}

void sendMessages(int sock) {
    std::string msg;
    while (true) {
        std::cout << "You: ";
        std::getline(std::cin, msg);
        if (msg == "exit") break;
        send(sock, msg.c_str(), msg.size(), 0);
    }
    close(sock);
    exit(0);
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    std::string server_ip;
    std::cout << "Enter server IP address: ";
    std::cin >> server_ip;
    std::cin.ignore();

    // 1. Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/Address not supported.\n";
        return 1;
    }

    // 2. Connect
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }

    std::cout << "Connected to server " << server_ip << ":" << PORT << "!\n";

    // 3. Threads for simultaneous send/receive
    std::thread recvThread(receiveMessages, sock);
    std::thread sendThread(sendMessages, sock);

    recvThread.join();
    sendThread.join();

    close(sock);
    return 0;
}
