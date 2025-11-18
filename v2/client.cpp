#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>

using namespace std;

const int BUF_SIZE = 4096;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "usage: " << argv[0] << " <ip> <port>\n";
        return 1;
    }

    string ip = argv[1];
    int port = stoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serv.sin_addr);

    connect(sock, (sockaddr *)&serv, sizeof(serv));
    cout << "connected to " << ip << ":" << port << "\n";

    fd_set fds;
    int maxfd = max(sock, STDIN_FILENO);

    while (true) {
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        FD_SET(STDIN_FILENO, &fds);

        select(maxfd + 1, &fds, nullptr, nullptr, nullptr);

        if (FD_ISSET(sock, &fds)) {
            char buf[BUF_SIZE];
            int n = recv(sock, buf, BUF_SIZE - 1, 0);
            if (n <= 0) break;
            buf[n] = 0;
            cout << buf;
        }

        if (FD_ISSET(STDIN_FILENO, &fds)) {
            string msg;
            getline(cin, msg);
            if (!cin) break;
            send(sock, msg.c_str(), msg.size(), 0);
        }
    }

    close(sock);
    return 0;
}
