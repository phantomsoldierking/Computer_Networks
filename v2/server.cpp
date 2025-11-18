#include <arpa/inet.h>
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

const int MAX_CLIENTS = 5;
const int BUF_SIZE = 4096;

string id_from_sockaddr(const sockaddr_in &sa) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sa.sin_addr, ip, sizeof(ip));
    return string(ip) + ":" + to_string(ntohs(sa.sin_port));
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

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(port);

    bind(listen_fd, (sockaddr *)&serv, sizeof(serv));
    listen(listen_fd, MAX_CLIENTS);

    cout << "server on port " << port << "\n";

    unordered_map<int, string> sock_to_id;
    unordered_map<string, int> id_to_sock;

    fd_set fds;

    while (true) {
        FD_ZERO(&fds);
        FD_SET(listen_fd, &fds);
        FD_SET(STDIN_FILENO, &fds);

        int maxfd = max(listen_fd, STDIN_FILENO);
        for (auto &p : sock_to_id) maxfd = max(maxfd, p.first);

        select(maxfd + 1, &fds, nullptr, nullptr, nullptr);

        // --- new connection ---
        if (FD_ISSET(listen_fd, &fds)) {
            sockaddr_in caddr{};
            socklen_t clen = sizeof(caddr);
            int cfd = accept(listen_fd, (sockaddr *)&caddr, &clen);

            if ((int)sock_to_id.size() >= MAX_CLIENTS) {
                send_msg(cfd, "server full\n");
                close(cfd);
                continue;
            }

            string id = id_from_sockaddr(caddr);
            sock_to_id[cfd] = id;
            id_to_sock[id] = cfd;

            cout << "connected: " << id << "\n";
            send_msg(cfd, "welcome " + id + "\n");
        }

        // --- admin commands ---
        if (FD_ISSET(STDIN_FILENO, &fds)) {
            string cmd;
            getline(cin, cmd);
            if (!cin) break;

            if (cmd.rfind("/send ", 0) == 0) {
                istringstream ss(cmd.substr(6));
                string id; ss >> id;
                string msg; getline(ss, msg);

                if (id_to_sock.count(id)) send_msg(id_to_sock[id], msg + "\n");
                else cout << "no such client\n";

            } else if (cmd.rfind("/broadcast ", 0) == 0) {
                string msg = cmd.substr(11);
                for (auto &p : sock_to_id) send_msg(p.first, msg + "\n");

            } else if (cmd == "/list") {
                for (auto &p : sock_to_id) cout << p.second << "\n";

            } else if (cmd == "/quit") {
                break;
            } else {
                cout << "unknown command\n";
            }
        }

        // --- messages from clients ---
        for (auto it = sock_to_id.begin(); it != sock_to_id.end();) {
            int cfd = it->first;

            if (FD_ISSET(cfd, &fds)) {
                char buf[BUF_SIZE];
                int n = recv(cfd, buf, BUF_SIZE - 1, 0);

                if (n <= 0) {
                    cout << "disconnect: " << it->second << "\n";
                    close(cfd);
                    id_to_sock.erase(it->second);
                    it = sock_to_id.erase(it);
                    continue;
                }

                buf[n] = 0;
                cout << "[" << it->second << "]: " << buf;
            }

            ++it;
        }
    }

    for (auto &p : sock_to_id) close(p.first);
    close(listen_fd);

    return 0;
}
