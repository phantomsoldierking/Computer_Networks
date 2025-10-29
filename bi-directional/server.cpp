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
            std::cout << "Client disconnected.\n";
            break;
        }
        std::cout << "Client: " << buffer << std::endl;
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
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // 1. Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // 2. Bind
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));

    // 3. Listen
    listen(server_fd, 3);
    std::cout << "Server listening on port " << PORT << "...\n";

    // 4. Accept client
    new_socket = accept(server_fd, (struct sockaddr*)&address,
                        (socklen_t*)&addrlen);
    std::cout << "Client connected!\n";

    // 5. Launch threads
    std::thread recvThread(receiveMessages, new_socket);
    std::thread sendThread(sendMessages, new_socket);

    recvThread.join();
    sendThread.join();

    close(new_socket);
    close(server_fd);
    return 0;
}
