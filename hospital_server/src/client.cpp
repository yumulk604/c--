#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

static bool recv_line(int fd, std::string &out) {
    out.clear();
    char ch;
    while (true) {
        ssize_t n = recv(fd, &ch, 1, 0);
        if (n == 0) return false;
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (ch == '\n') break;
        out.push_back(ch);
        if (out.size() > 4096) return false;
    }
    return true;
}

static bool send_line(int fd, const std::string &s) {
    std::string with_nl = s + "\n";
    const char *p = with_nl.c_str();
    size_t left = with_nl.size();
    while (left > 0) {
        ssize_t n = send(fd, p, left, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        p += n;
        left -= (size_t)n;
    }
    return true;
}

int main(int argc, char **argv) {
    const char *ip = argc > 1 ? argv[1] : "127.0.0.1";
    int port = argc > 2 ? std::stoi(argv[2]) : 13000;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }

    std::string line;
    if (!recv_line(fd, line)) { std::cerr << "No banner" << std::endl; return 1; }
    std::cout << line << std::endl;
    send_line(fd, "HELLO 1");
    if (!recv_line(fd, line)) { std::cerr << "No response" << std::endl; return 1; }
    std::cout << line << std::endl;

    // Simple scripted test
    send_line(fd, "PING");
    recv_line(fd, line); std::cout << line << std::endl;

    send_line(fd, "ADD_PATIENT John_Doe 1990-01-01");
    recv_line(fd, line); std::cout << line << std::endl;

    send_line(fd, "LIST_PATIENTS");
    recv_line(fd, line); std::cout << line << std::endl;
    if (line.rfind("OK ", 0) == 0) {
        int count = std::stoi(line.substr(3));
        for (int i = 0; i < count; ++i) { recv_line(fd, line); std::cout << line << std::endl; }
    }

    send_line(fd, "QUIT");
    recv_line(fd, line); std::cout << line << std::endl;

    close(fd);
    return 0;
}
