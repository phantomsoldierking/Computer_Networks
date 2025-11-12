#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

static const int max_clients = 5;
static const int buf_size = 4096;

std::string sockaddr_to_id(const sockaddr_in &sa) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sa.sin_addr, ip, sizeof(ip));
    std::ostringstream ss;
    ss << ip << ":" << ntohs(sa.sin_port);
    return ss.str();
}

void send_msg(int sock, const std::string &msg) {
    send(sock, msg.c_str(), msg.size(), 0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <port>\n";
        return 1;
    }

    int port = std::stoi(argv[1]);
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(listen_fd, max_clients) < 0) {
        perror("listen");
        return 1;
    }

    std::cout << "server listening on port " << port << "\n";
    std::cout << "commands: /send <ip:port> <msg> | /broadcast <msg> | /list | /quit\n";

    std::unordered_map<std::string, int> id_to_sock;
    std::unordered_map<int, std::string> sock_to_id;

    fd_set readfds;
    int maxfd = listen_fd;

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(listen_fd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        maxfd = std::max(maxfd, STDIN_FILENO);

        for (auto &p : id_to_sock) {
            FD_SET(p.second, &readfds);
            if (p.second > maxfd) maxfd = p.second;
        }

        if (select(maxfd + 1, &readfds, nullptr, nullptr, nullptr) < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        // new connection
        if (FD_ISSET(listen_fd, &readfds)) {
            sockaddr_in caddr{};
            socklen_t clen = sizeof(caddr);
            int cfd = accept(listen_fd, (sockaddr *)&caddr, &clen);
            if (cfd >= 0) {
                std::string id = sockaddr_to_id(caddr);
                if ((int)id_to_sock.size() >= max_clients) {
                    std::string msg = "server full\n";
                    send_msg(cfd, msg);
                    close(cfd);
                } else {
                    id_to_sock[id] = cfd;
                    sock_to_id[cfd] = id;
                    std::cout << "connected: " << id << "\n";
                    send_msg(cfd, "welcome " + id + "\n");
                }
            }
        }

        // stdin command
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            std::string line;
            std::getline(std::cin, line);
            if (!std::cin) break;
            if (line.empty()) continue;

            if (line.rfind("/send ", 0) == 0) {
                std::istringstream iss(line.substr(6));
                std::string id;
                iss >> id;
                std::string msg;
                std::getline(iss, msg);
                if (id_to_sock.count(id)) {
                    send_msg(id_to_sock[id], "server: " + msg + "\n");
                    std::cout << "sent to " << id << "\n";
                } else {
                    std::cout << "no such client: " << id << "\n";
                }
            } else if (line.rfind("/broadcast ", 0) == 0) {
                std::string msg = line.substr(11);
                for (auto &p : id_to_sock)
                    send_msg(p.second, "broadcast: " + msg + "\n");
            } else if (line == "/list") {
                std::cout << "clients (" << id_to_sock.size() << "):\n";
                for (auto &p : id_to_sock)
                    std::cout << "  " << p.first << "\n";
            } else if (line == "/quit") {
                std::cout << "server shutting down\n";
                break;
            } else {
                std::cout << "unknown command\n";
            }
        }

        // read from clients
        std::vector<int> to_remove;
        for (auto &p : id_to_sock) {
            int cfd = p.second;
            if (FD_ISSET(cfd, &readfds)) {
                char buf[buf_size];
                ssize_t n = recv(cfd, buf, sizeof(buf) - 1, 0);
                if (n > 0) {
                    buf[n] = '\0';
                    std::cout << "[" << p.first << "]: " << buf;
                    // echo back
                    send_msg(cfd, "server echo: " + std::string(buf));
                } else {
                    std::cout << "disconnected: " << p.first << "\n";
                    to_remove.push_back(cfd);
                }
            }
        }

        for (int cfd : to_remove) {
            close(cfd);
            std::string id = sock_to_id[cfd];
            sock_to_id.erase(cfd);
            id_to_sock.erase(id);
        }
    }

    for (auto &p : id_to_sock) close(p.second);
    close(listen_fd);
    return 0;
}
