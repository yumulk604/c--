#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <ctime>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "database.h"
#include "protocol.h"

namespace {
std::atomic<bool> g_stop{false};

uint32_t get_unix_time() { return static_cast<uint32_t>(time(nullptr)); }

void on_signal(int) { g_stop = true; }

std::string trim_newlines(const std::string &s) {
    size_t start = 0;
    while (start < s.size() && (s[start] == '\r' || s[start] == '\n')) ++start;
    size_t end = s.size();
    while (end > start && (s[end - 1] == '\r' || s[end - 1] == '\n')) --end;
    return s.substr(start, end - start);
}

bool recv_line(int fd, std::string &out) {
    out.clear();
    char ch;
    while (true) {
        ssize_t n = recv(fd, &ch, 1, 0);
        if (n == 0) return false; // disconnect
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (ch == '\n') break;
        out.push_back(ch);
        if (out.size() > 4096) return false; // prevent overly long lines
    }
    return true;
}

bool send_line(int fd, const std::string &s) {
    std::string with_nl = s;
    with_nl.push_back('\n');
    const char *data = with_nl.c_str();
    size_t left = with_nl.size();
    while (left > 0) {
        ssize_t n = send(fd, data, left, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        data += n;
        left -= static_cast<size_t>(n);
    }
    return true;
}

void client_thread(int client_fd, sockaddr_in client_addr, Database &db) {
    char ipbuf[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &client_addr.sin_addr, ipbuf, sizeof(ipbuf));

    // Minimal handshake
    // Expect: HELLO <client_version>
    // Respond: WELCOME <server_time>
    if (!send_line(client_fd, "WELCOME " + std::to_string(get_unix_time()))) {
        close(client_fd);
        return;
    }
    std::string line;
    if (!recv_line(client_fd, line)) {
        close(client_fd);
        return;
    }
    line = trim_newlines(line);
    auto parts = proto::split_ws(line);
    if (parts.empty() || parts[0] != "HELLO") {
        send_line(client_fd, "ERR expected HELLO");
        close(client_fd);
        return;
    }

    send_line(client_fd, "OK type HELP for commands");

    while (!g_stop) {
        if (!send_line(client_fd, "> ")) break; // prompt
        if (!recv_line(client_fd, line)) break;
        line = trim_newlines(line);
        auto tokens = proto::split_ws(line);
        if (tokens.empty()) continue;
        std::string cmd = tokens[0];

        try {
            if (cmd == "QUIT") {
                send_line(client_fd, "BYE");
                break;
            } else if (cmd == "PING") {
                send_line(client_fd, "PONG " + std::to_string(get_unix_time()));
            } else if (cmd == "HELP") {
                send_line(client_fd, "Commands: PING, ADD_PATIENT <name> <dob>, LIST_PATIENTS, ADD_APPOINTMENT <patient_id> <when_ts> <notes...>, LIST_APPOINTMENTS <patient_id>, QUIT");
            } else if (cmd == "ADD_PATIENT") {
                if (tokens.size() < 3) {
                    send_line(client_fd, "ERR usage: ADD_PATIENT <name> <dob>");
                    continue;
                }
                int id = db.addPatient(tokens[1], tokens[2]);
                send_line(client_fd, "OK PATIENT " + std::to_string(id));
            } else if (cmd == "LIST_PATIENTS") {
                auto patients = db.listPatients();
                if (patients.empty()) {
                    send_line(client_fd, "OK 0");
                } else {
                    send_line(client_fd, "OK " + std::to_string(patients.size()));
                    for (const auto &p : patients) {
                        int id; std::string name, dob; std::tie(id, name, dob) = p;
                        send_line(client_fd, std::to_string(id) + " " + name + " " + dob);
                    }
                }
            } else if (cmd == "ADD_APPOINTMENT") {
                if (tokens.size() < 4) {
                    send_line(client_fd, "ERR usage: ADD_APPOINTMENT <patient_id> <when_ts> <notes...>");
                    continue;
                }
                int pid = std::stoi(tokens[1]);
                std::string when_ts = tokens[2];
                std::vector<std::string> rest(tokens.begin() + 3, tokens.end());
                std::string notes = proto::join_space(rest);
                int id = db.addAppointment(pid, when_ts, notes);
                send_line(client_fd, "OK APPT " + std::to_string(id));
            } else if (cmd == "LIST_APPOINTMENTS") {
                if (tokens.size() < 2) {
                    send_line(client_fd, "ERR usage: LIST_APPOINTMENTS <patient_id>");
                    continue;
                }
                int pid = std::stoi(tokens[1]);
                auto appts = db.listAppointments(pid);
                send_line(client_fd, "OK " + std::to_string(appts.size()));
                for (const auto &a : appts) {
                    int id, patient; std::string when_ts, notes; std::tie(id, patient, when_ts, notes) = a;
                    send_line(client_fd, std::to_string(id) + " " + when_ts + " " + notes);
                }
            } else {
                send_line(client_fd, "ERR unknown command");
            }
        } catch (const std::exception &ex) {
            send_line(client_fd, std::string("ERR ") + ex.what());
        }
    }

    close(client_fd);
}

} // namespace

int main(int argc, char **argv) {
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    const char *listen_ip = argc > 1 ? argv[1] : "0.0.0.0";
    int listen_port = argc > 2 ? std::stoi(argv[2]) : 13000;
    const char *db_path = argc > 3 ? argv[3] : "hospital.db";

    try {
        Database db(db_path);

        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) {
            perror("socket");
            return 1;
        }

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(listen_port));
        if (inet_pton(AF_INET, listen_ip, &addr.sin_addr) != 1) {
            std::cerr << "Invalid IP: " << listen_ip << "\n";
            return 1;
        }

        if (bind(server_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) {
            perror("bind");
            return 1;
        }
        if (listen(server_fd, 16) == -1) {
            perror("listen");
            return 1;
        }

        std::cout << "Hospital server listening on " << listen_ip << ":" << listen_port << "\n";

        std::vector<std::thread> workers;
        while (!g_stop) {
            sockaddr_in caddr{};
            socklen_t clen = sizeof(caddr);
            int client_fd = accept(server_fd, reinterpret_cast<sockaddr *>(&caddr), &clen);
            if (client_fd < 0) {
                if (errno == EINTR) continue;
                perror("accept");
                break;
            }
            // Spawn detached thread per client
            workers.emplace_back([client_fd, caddr, &db]() mutable {
                client_thread(client_fd, caddr, db);
            });
            workers.back().detach();
        }

        close(server_fd);
    } catch (const std::exception &ex) {
        std::cerr << "Fatal: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
