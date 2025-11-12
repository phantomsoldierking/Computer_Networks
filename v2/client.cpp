#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>

using namespace std;

static const int buf_size = 4096;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "usage: " << argv[0] << " <server_ip> <port>\n";
        return 1;
    }

    string server_ip = argv[1];
    int port = stoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    inet_pton(AF_INET, server_ip.c_str(), &serv.sin_addr);

    if (connect(sock, (sockaddr *)&serv, sizeof(serv)) < 0) {
        perror("connect");
        return 1;
    }

    cout << "connected to " << server_ip << ":" << port << "\n";
    cout << "type messages, Ctrl+C to quit\n";

    fd_set readfds;
    int maxfd = max(sock, STDIN_FILENO);
    char buf[buf_size];

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        if (select(maxfd + 1, &readfds, nullptr, nullptr, nullptr) < 0) {
            perror("select");
            break;
        }

        // message from server
        if (FD_ISSET(sock, &readfds)) {
            ssize_t n = recv(sock, buf, sizeof(buf) - 1, 0);
            if (n > 0) {
                buf[n] = '\0';
                cout << buf;
                cout.flush();
            } else {
                cout << "server closed connection\n";
                break;
            }
        }

        // message from user input
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            string line;
            getline(cin, line);
            if (!cin) break;
            if (!line.empty()) send(sock, line.c_str(), line.size(), 0);
        }
    }

    close(sock);
    return 0;
}
