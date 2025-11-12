#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

using namespace std;

const int max_clients = 5;
const int buf_size = 4096;

string sockaddr_to_id(const sockaddr_in &sa) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sa.sin_addr, ip, sizeof(ip));
    ostringstream ss;
    ss << ip << ":" << ntohs(sa.sin_port);
    return ss.str();
}

void send_msg(int sock, const string &msg) {
    send(sock, msg.c_str(), msg.size(), 0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "usage: " << argv[0] << " <port>\n";
        return 1;
    }

    int port = stoi(argv[1]);
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

    cout << "server listening on port " << port << "\n";
    cout << "commands: /send <ip:port> <msg> | /broadcast <msg> | /list | /quit\n";

    unordered_map<string, int> id_to_sock;
    unordered_map<int, string> sock_to_id;

    fd_set readfds;
    int maxfd = listen_fd;

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(listen_fd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        maxfd = max(maxfd, STDIN_FILENO);

        for (auto &p : id_to_sock) {
            FD_SET(p.second, &readfds);
            if (p.second > maxfd) maxfd = p.second;
        }

        if (select(maxfd + 1, &readfds, nullptr, nullptr, nullptr) < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        // new client connection
        if (FD_ISSET(listen_fd, &readfds)) {
            sockaddr_in caddr{};
            socklen_t clen = sizeof(caddr);
            int cfd = accept(listen_fd, (sockaddr *)&caddr, &clen);
            if (cfd >= 0) {
                string id = sockaddr_to_id(caddr);
                if ((int)id_to_sock.size() >= max_clients) {
                    send_msg(cfd, "server full\n");
                    close(cfd);
                } else {
                    id_to_sock[id] = cfd;
                    sock_to_id[cfd] = id;
                    cout << "connected: " << id << "\n";
                    send_msg(cfd, "Hi " + id + ", welcome!\n");
                }
            }
        }

        // stdin commands for admin
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            string line;
            getline(cin, line);
            if (!cin) break;
            if (line.empty()) continue;

            if (line.rfind("/send ", 0) == 0) {
                istringstream iss(line.substr(6));
                string id;
                iss >> id;
                string msg;
                getline(iss, msg);
                if (id_to_sock.count(id)) {
                    send_msg(id_to_sock[id], "server: " + msg + "\n");
                    cout << "sent to " << id << "\n";
                } else {
                    cout << "no such client: " << id << "\n";
                }
            } else if (line.rfind("/broadcast ", 0) == 0) {
                string msg = line.substr(11);
                for (auto &p : id_to_sock)
                    send_msg(p.second, "broadcast: " + msg + "\n");
            } else if (line == "/list") {
                cout << "clients (" << id_to_sock.size() << "):\n";
                for (auto &p : id_to_sock)
                    cout << "  " << p.first << "\n";
            } else if (line == "/quit") {
                cout << "server shutting down\n";
                break;
            } else {
                cout << "unknown command\n";
            }
        }

        // messages from clients
        for (auto &p : id_to_sock) {
            int cfd = p.second;
            if (FD_ISSET(cfd, &readfds)) {
                char buf[buf_size];
                ssize_t n = recv(cfd, buf, sizeof(buf) - 1, 0);
                if (n > 0) {
                    buf[n] = '\0';
                    cout << "[" << sock_to_id[cfd] << "]: " << buf << "\n";
                } else {
                    cout << "client disconnected: " << sock_to_id[cfd] << "\n";
                    close(cfd);
                    id_to_sock.erase(sock_to_id[cfd]);
                    sock_to_id.erase(cfd);
                    break;
                }
            }
        }
    }

    for (auto &p : id_to_sock) close(p.second);
    close(listen_fd);
    return 0;
}
