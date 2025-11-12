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
        std::cout << "\nClient: " << buffer << std::endl;
        std::cout << "Server(You): " << std::flush;
    }
}

void sendMessages(int socket) {
    std::string msg;
    while (true) {
        std::cout << "Server(You): ";
        std::getline(std::cin, msg);
        if (msg == "exit") break;
        send(socket, msg.c_str(), msg.size(), 0);
    }
    close(socket);
    exit(0);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    std::string ip;
    std::cout << "Enter server IP to bind (usually your local IP, e.g. 192.168.x.x): ";
    std::cin >> ip;
    std::cin.ignore();  // clear newline from input buffer

    // 1. Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket failed");
        return 1;
    }

    // Allow reusing the port quickly
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    // 2. Bind
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    inet_pton(AF_INET, ip.c_str(), &address.sin_addr);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }

    // 3. Listen
    listen(server_fd, 3);
    std::cout << "Server listening on " << ip << ":" << PORT << "...\n";

    // 4. Accept
    new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (new_socket < 0) {
        perror("Accept failed");
        return 1;
    }

    std::cout << "Client connected!\n";

    // 5. Threads for communication
    std::thread recvThread(receiveMessages, new_socket);
    std::thread sendThread(sendMessages, new_socket);

    recvThread.join();
    sendThread.join();

    close(new_socket);
    close(server_fd);
    return 0;
}
