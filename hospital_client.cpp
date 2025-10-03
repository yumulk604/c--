#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

bool sendAll(int fd, const char* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(fd, data + sent, len - sent, 0);
        if (n <= 0) {
            if (errno == EINTR) continue;
            return false;
        }
        sent += static_cast<size_t>(n);
    }
    return true;
}

bool sendLine(int fd, const std::string& line) {
    std::string out = line; out.push_back('\n');
    return sendAll(fd, out.c_str(), out.size());
}

class LineReader {
public:
    explicit LineReader(int fd) : fd_(fd) {}
    bool readLine(std::string& out) {
        for (;;) {
            auto pos = buffer_.find('\n');
            if (pos != std::string::npos) {
                out = buffer_.substr(0, pos);
                buffer_.erase(0, pos + 1);
                return true;
            }
            char tmp[1024];
            ssize_t n = ::recv(fd_, tmp, sizeof(tmp), 0);
            if (n == 0) return false;
            if (n < 0) { if (errno == EINTR) continue; return false; }
            buffer_.append(tmp, static_cast<size_t>(n));
        }
    }
private:
    int fd_;
    std::string buffer_;
};

int main(int argc, char** argv) {
    const char* host = "127.0.0.1";
    int port = 13000;
    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = std::stoi(argv[2]);

    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { std::cerr << "socket: " << std::strerror(errno) << "\n"; return 1; }

    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, host, &addr.sin_addr) != 1) { std::cerr << "inet_pton failed\n"; return 1; }

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "connect: " << std::strerror(errno) << "\n"; return 1;
    }

    LineReader reader(fd);
    std::string line;
    if (reader.readLine(line)) std::cout << line << "\n";

    std::cout << "Type commands (HELP for list). Ctrl+D to exit." << std::endl;

    std::string input;
    while (std::cout << "> " && std::getline(std::cin, input)) {
        if (!sendLine(fd, input)) { std::cerr << "send failed\n"; break; }
        // Print until a terminating condition: either single-line, or END marker for multi-line
        do {
            if (!reader.readLine(line)) { std::cerr << "server closed\n"; goto done; }
            std::cout << line << "\n";
        } while (line != "END" && input.rfind("LIST_", 0) == 0);
        if (input == "QUIT") break;
    }

 done:
    ::close(fd);
    return 0;
}
