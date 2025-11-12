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
    std::cin.ignore();

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket failed");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) <= 0) {
        std::cerr << "Invalid IP address.\n";
        return 1;
    }

    // Bind to IP, fallback to INADDR_ANY if local interface doesn't match
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed, retrying with INADDR_ANY");
        address.sin_addr.s_addr = INADDR_ANY;
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            perror("Bind failed again");
            return 1;
        }
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        return 1;
    }

    std::cout << "Server listening on " << ip << ":" << PORT << "...\n";

    // Wait up to 30 seconds for client connection
    fd_set set;
    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;

    FD_ZERO(&set);
    FD_SET(server_fd, &set);

    std::cout << "Waiting for a client to connect (30s timeout)...\n";
    int rv = select(server_fd + 1, &set, NULL, NULL, &timeout);

    if (rv == -1) {
        perror("Select error");
        return 1;
    } else if (rv == 0) {
        std::cout << "No client connected within 30 seconds. Server shutting down.\n";
        close(server_fd);
        return 0;
    }

    new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (new_socket < 0) {
        perror("Accept failed");
        return 1;
    }

    std::cout << "Client connected!\n";

    std::thread recvThread(receiveMessages, new_socket);
    std::thread sendThread(sendMessages, new_socket);

    recvThread.join();
    sendThread.join();

    close(new_socket);
    close(server_fd);
    return 0;
}
