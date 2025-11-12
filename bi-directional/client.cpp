#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <chrono>

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
        std::cout << "\nServer: " << buffer << std::endl;
        std::cout << "Client(you): " << std::flush;
    }
}

void sendMessages(int socket) {
    std::string msg;
    while (true) {
        std::cout << "Client(You): ";
        std::getline(std::cin, msg);
        if (msg == "exit") break;
        send(socket, msg.c_str(), msg.size(), 0);
    }
    close(socket);
    exit(0);
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    std::string server_ip;
    std::cout << "Enter server IP address to connect: ";
    std::cin >> server_ip;
    std::cin.ignore();

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

    // Retry connection for up to 30 seconds
    bool connected = false;
    for (int i = 0; i < 30; ++i) {
        if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == 0) {
            connected = true;
            break;
        }
        std::cout << "Connection failed, retrying (" << i + 1 << "/30)...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (!connected) {
        std::cerr << "Unable to connect to server after 30 seconds.\n";
        close(sock);
        return 1;
    }

    std::cout << "Connected to server at " << server_ip << ":" << PORT << "!\n";

    std::thread recvThread(receiveMessages, sock);
    std::thread sendThread(sendMessages, sock);

    recvThread.join();
    sendThread.join();

    close(sock);
    return 0;
}
