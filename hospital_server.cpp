#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include <memory>
#include <dlfcn.h>

namespace {

constexpr int kDefaultPort = 13000;
const char* kDefaultDbPath = "hospital.db";

std::atomic<bool> g_shouldStop{false};

void handleSignal(int) {
    g_shouldStop.store(true);
}

// Minimal forward declarations to avoid sqlite3 headers
struct sqlite3;
struct sqlite3_stmt;

// Selected SQLite result codes we need
enum { SQLITE_OK = 0, SQLITE_ROW = 100, SQLITE_DONE = 101 };

// Dynamically loaded SQLite API
struct SqliteApi {
    void* handle = nullptr;
    int (*open)(const char*, sqlite3**) = nullptr;
    int (*close)(sqlite3*) = nullptr;
    int (*exec)(sqlite3*, const char*, int (*)(void*,int,char**,char**), void*, char**) = nullptr;
    const char* (*errmsg)(sqlite3*) = nullptr;
    void (*free_)(void*) = nullptr;
    int (*prepare_v2)(sqlite3*, const char*, int, sqlite3_stmt**, const char**) = nullptr;
    int (*bind_text)(sqlite3_stmt*, int, const char*, int, void(*)(void*)) = nullptr;
    int (*bind_int)(sqlite3_stmt*, int, int) = nullptr;
    int (*step)(sqlite3_stmt*) = nullptr;
    int (*column_int)(sqlite3_stmt*, int) = nullptr;
    const unsigned char* (*column_text)(sqlite3_stmt*, int) = nullptr;
    long long (*last_insert_rowid)(sqlite3*) = nullptr;
    int (*finalize)(sqlite3_stmt*) = nullptr;
};

void* requireSym(void* handle, const char* name) {
    void* sym = dlsym(handle, name);
    if (!sym) {
        throw std::runtime_error(std::string("Missing symbol ") + name + ": " + dlerror());
    }
    return sym;
}

SqliteApi& sqlite() {
    static std::once_flag flag;
    static SqliteApi api;
    std::call_once(flag, []() {
        const char* candidates[] = { "libsqlite3.so.0", "libsqlite3.so" };
        void* handle = nullptr;
        for (const char* c : candidates) {
            handle = dlopen(c, RTLD_NOW | RTLD_LOCAL);
            if (handle) break;
        }
        if (!handle) {
            throw std::runtime_error(std::string("dlopen sqlite3 failed: ") + dlerror());
        }
        api.handle = handle;
        api.open = reinterpret_cast<int(*)(const char*, sqlite3**)>(requireSym(handle, "sqlite3_open"));
        api.close = reinterpret_cast<int(*)(sqlite3*)>(requireSym(handle, "sqlite3_close"));
        api.exec = reinterpret_cast<int(*)(sqlite3*, const char*, int (*)(void*,int,char**,char**), void*, char**)>(requireSym(handle, "sqlite3_exec"));
        api.errmsg = reinterpret_cast<const char*(*)(sqlite3*)>(requireSym(handle, "sqlite3_errmsg"));
        api.free_ = reinterpret_cast<void(*)(void*)>(requireSym(handle, "sqlite3_free"));
        api.prepare_v2 = reinterpret_cast<int(*)(sqlite3*, const char*, int, sqlite3_stmt**, const char**)>(requireSym(handle, "sqlite3_prepare_v2"));
        api.bind_text = reinterpret_cast<int(*)(sqlite3_stmt*, int, const char*, int, void(*)(void*))>(requireSym(handle, "sqlite3_bind_text"));
        api.bind_int = reinterpret_cast<int(*)(sqlite3_stmt*, int, int)>(requireSym(handle, "sqlite3_bind_int"));
        api.step = reinterpret_cast<int(*)(sqlite3_stmt*)>(requireSym(handle, "sqlite3_step"));
        api.column_int = reinterpret_cast<int(*)(sqlite3_stmt*, int)>(requireSym(handle, "sqlite3_column_int"));
        api.column_text = reinterpret_cast<const unsigned char*(*)(sqlite3_stmt*, int)>(requireSym(handle, "sqlite3_column_text"));
        api.last_insert_rowid = reinterpret_cast<long long(*)(sqlite3*)>(requireSym(handle, "sqlite3_last_insert_rowid"));
        api.finalize = reinterpret_cast<int(*)(sqlite3_stmt*)>(requireSym(handle, "sqlite3_finalize"));
    });
    return api;
}

struct SqliteDeleter {
    void operator()(sqlite3* db) const noexcept { if (db) sqlite().close(db); }
    void operator()(sqlite3_stmt* stmt) const noexcept { if (stmt) sqlite().finalize(stmt); }
};

using SqliteDbPtr = std::unique_ptr<sqlite3, SqliteDeleter>;
using SqliteStmtPtr = std::unique_ptr<sqlite3_stmt, SqliteDeleter>;

int execOrThrow(sqlite3* db, const char* sql) {
    char* errMsg = nullptr;
    int rc = sqlite().exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string msg = errMsg ? errMsg : std::string("unknown sqlite error");
        if (errMsg) sqlite().free_(errMsg);
        throw std::runtime_error("SQLite exec failed: " + msg);
    }
    return rc;
}

SqliteDbPtr openDatabase(const std::string& path) {
    sqlite3* rawDb = nullptr;
    if (sqlite().open(path.c_str(), &rawDb) != SQLITE_OK) {
        std::string msg = rawDb ? sqlite().errmsg(rawDb) : "(null)";
        if (rawDb) sqlite().close(rawDb);
        throw std::runtime_error("Failed to open database: " + msg);
    }

    SqliteDbPtr db(rawDb);
    execOrThrow(db.get(), "PRAGMA journal_mode=WAL;");
    execOrThrow(db.get(), "PRAGMA foreign_keys=ON;");

    execOrThrow(db.get(),
        "CREATE TABLE IF NOT EXISTS patients ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL,"
        "  age INTEGER NOT NULL CHECK(age >= 0)"
        ");");

    execOrThrow(db.get(),
        "CREATE TABLE IF NOT EXISTS appointments ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  patient_id INTEGER NOT NULL,"
        "  date TEXT NOT NULL,"
        "  note TEXT,"
        "  FOREIGN KEY(patient_id) REFERENCES patients(id) ON DELETE CASCADE"
        ");");

    return db;
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::string item;
    std::istringstream iss(s);
    while (std::getline(iss, item, delim)) {
        parts.push_back(item);
    }
    return parts;
}

std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) start++;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) end--;
    return s.substr(start, end - start);
}

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
    std::string out = line;
    out.push_back('\n');
    return sendAll(fd, out.c_str(), out.size());
}

// Simple buffered line reader over a socket fd
class LineReader {
public:
    explicit LineReader(int fd) : fd_(fd) {}

    // Returns false on EOF or error without producing a line
    bool readLine(std::string& out) {
        for (;;) {
            // Check if there is a newline in the buffer
            auto pos = buffer_.find('\n');
            if (pos != std::string::npos) {
                out = buffer_.substr(0, pos);
                buffer_.erase(0, pos + 1);
                return true;
            }

            char temp[1024];
            ssize_t n = ::recv(fd_, temp, sizeof(temp), 0);
            if (n == 0) {
                // peer closed
                return false;
            }
            if (n < 0) {
                if (errno == EINTR) continue;
                return false;
            }
            buffer_.append(temp, static_cast<size_t>(n));
        }
    }

private:
    int fd_;
    std::string buffer_;
};

struct ServerOptions {
    int port = kDefaultPort;
    std::string dbPath = kDefaultDbPath;
    bool once = false; // accept one connection then exit
};

void printHelp() {
    std::cout << "Hospital Server\n";
    std::cout << "Options: --port <p> --db <path> [--once]\n";
}

ServerOptions parseArgs(int argc, char** argv) {
    ServerOptions opt;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--port" && i + 1 < argc) {
            opt.port = std::stoi(argv[++i]);
        } else if (a == "--db" && i + 1 < argc) {
            opt.dbPath = argv[++i];
        } else if (a == "--once") {
            opt.once = true;
        } else if (a == "-h" || a == "--help") {
            printHelp();
            std::exit(0);
        } else {
            std::cerr << "Unknown arg: " << a << "\n";
            printHelp();
            std::exit(2);
        }
    }
    return opt;
}

std::mutex g_dbMutex; // serialize writes for simplicity

std::string escapeSql(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
        if (c == '\'') out += "''";
        else out.push_back(c);
    }
    return out;
}

bool cmdAddPatient(sqlite3* db, const std::string& name, int age, int& outId) {
    std::lock_guard<std::mutex> lock(g_dbMutex);
    const char* sql = "INSERT INTO patients(name, age) VALUES(?, ?)";
    SqliteStmtPtr stmt;
    {
        sqlite3_stmt* raw = nullptr;
        if (sqlite().prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK) {
            std::cerr << "prepare_v2 failed (ADD_PATIENT): " << sqlite().errmsg(db) << "\n";
            // Fallback to EXEC composed SQL
            try {
                std::ostringstream oss;
                oss << "INSERT INTO patients(name, age) VALUES('" << escapeSql(name) << "', " << age << ")";
                execOrThrow(db, oss.str().c_str());
                outId = static_cast<int>(sqlite().last_insert_rowid(db));
                return true;
            } catch (const std::exception& e) {
                std::cerr << "fallback exec failed (ADD_PATIENT): " << e.what() << "\n";
                return false;
            }
        }
        stmt.reset(raw);
    }
    if (sqlite().bind_text(stmt.get(), 1, name.c_str(), -1, reinterpret_cast<void(*)(void*)>(-1)) != SQLITE_OK) {
        std::cerr << "bind_text failed (name)" << "\n";
        return false;
    }
    if (sqlite().bind_int(stmt.get(), 2, age) != SQLITE_OK) {
        std::cerr << "bind_int failed (age)" << "\n";
        return false;
    }
    if (sqlite().step(stmt.get()) != SQLITE_DONE) {
        std::cerr << "step failed (ADD_PATIENT): " << sqlite().errmsg(db) << "\n";
        return false;
    }
    outId = static_cast<int>(sqlite().last_insert_rowid(db));
    return true;
}

bool cmdListPatients(sqlite3* db, std::vector<std::tuple<int,std::string,int>>& out) {
    const char* sql = "SELECT id, name, age FROM patients ORDER BY id";
    SqliteStmtPtr stmt;
    {
        sqlite3_stmt* raw = nullptr;
        if (sqlite().prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK) {
            std::cerr << "prepare_v2 failed (LIST_PATIENTS): " << sqlite().errmsg(db) << "\n";
            // Fallback to sqlite3_exec with callback
            try {
                auto cb = [](void* data, int argc, char** argv, char** colname) -> int {
                    (void)argc; (void)colname;
                    auto* vec = reinterpret_cast<std::vector<std::tuple<int,std::string,int>>*>(data);
                    int id = argv[0] ? std::atoi(argv[0]) : 0;
                    std::string name = argv[1] ? argv[1] : "";
                    int age = argv[2] ? std::atoi(argv[2]) : 0;
                    vec->emplace_back(id, name, age);
                    return 0;
                };
                char* err = nullptr;
                int rc = sqlite().exec(db, sql, cb, &out, &err);
                if (rc != SQLITE_OK) {
                    std::string msg = err ? err : "unknown";
                    if (err) sqlite().free_(err);
                    std::cerr << "exec failed (LIST_PATIENTS): " << msg << "\n";
                    return false;
                }
                return true;
            } catch (const std::exception& e) {
                std::cerr << "fallback exec failed (LIST_PATIENTS): " << e.what() << "\n";
                return false;
            }
        }
        stmt.reset(raw);
    }
    for (;;) {
        int rc = sqlite().step(stmt.get());
        if (rc == SQLITE_ROW) {
            int id = sqlite().column_int(stmt.get(), 0);
            const unsigned char* n = sqlite().column_text(stmt.get(), 1);
            int age = sqlite().column_int(stmt.get(), 2);
            out.emplace_back(id, std::string(reinterpret_cast<const char*>(n ? n : reinterpret_cast<const unsigned char*>(""))), age);
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::cerr << "step failed (LIST_PATIENTS): " << sqlite().errmsg(db) << "\n";
            return false;
        }
    }
    return true;
}

bool cmdGetPatient(sqlite3* db, int id, std::tuple<int,std::string,int>& out, bool& found) {
    const char* sql = "SELECT id, name, age FROM patients WHERE id = ?";
    SqliteStmtPtr stmt;
    {
        sqlite3_stmt* raw = nullptr;
        if (sqlite().prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK) {
            std::cerr << "prepare_v2 failed (GET_PATIENT): " << sqlite().errmsg(db) << "\n";
            return false;
        }
        stmt.reset(raw);
    }
    if (sqlite().bind_int(stmt.get(), 1, id) != SQLITE_OK) {
        std::cerr << "bind_int failed (id)" << "\n";
        return false;
    }
    int rc = sqlite().step(stmt.get());
    if (rc == SQLITE_ROW) {
        int gotId = sqlite().column_int(stmt.get(), 0);
        const unsigned char* n = sqlite().column_text(stmt.get(), 1);
        int age = sqlite().column_int(stmt.get(), 2);
        out = std::make_tuple(gotId, std::string(reinterpret_cast<const char*>(n ? n : reinterpret_cast<const unsigned char*>(""))), age);
        found = true;
        return true;
    } else if (rc == SQLITE_DONE) {
        found = false;
        return true;
    }
    return false;
}

bool cmdAddAppointment(sqlite3* db, int patientId, const std::string& date, const std::string& note, int& outId) {
    std::lock_guard<std::mutex> lock(g_dbMutex);
    const char* sql = "INSERT INTO appointments(patient_id, date, note) VALUES(?, ?, ?)";
    SqliteStmtPtr stmt;
    {
        sqlite3_stmt* raw = nullptr;
        if (sqlite().prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK) {
            std::cerr << "prepare_v2 failed (ADD_APPOINTMENT): " << sqlite().errmsg(db) << "\n";
            return false;
        }
        stmt.reset(raw);
    }
    if (sqlite().bind_int(stmt.get(), 1, patientId) != SQLITE_OK) { std::cerr << "bind_int failed (patientId)\n"; return false; }
    if (sqlite().bind_text(stmt.get(), 2, date.c_str(), -1, reinterpret_cast<void(*)(void*)>(-1)) != SQLITE_OK) { std::cerr << "bind_text failed (date)\n"; return false; }
    if (sqlite().bind_text(stmt.get(), 3, note.c_str(), -1, reinterpret_cast<void(*)(void*)>(-1)) != SQLITE_OK) { std::cerr << "bind_text failed (note)\n"; return false; }
    if (sqlite().step(stmt.get()) != SQLITE_DONE) { std::cerr << "step failed (ADD_APPOINTMENT): " << sqlite().errmsg(db) << "\n"; return false; }
    outId = static_cast<int>(sqlite().last_insert_rowid(db));
    return true;
}

bool cmdListAppointments(sqlite3* db, int patientId, std::vector<std::tuple<int,std::string,std::string>>& out) {
    const char* sql = "SELECT id, date, note FROM appointments WHERE patient_id = ? ORDER BY id";
    SqliteStmtPtr stmt;
    {
        sqlite3_stmt* raw = nullptr;
        if (sqlite().prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK) {
            std::cerr << "prepare_v2 failed (LIST_APPOINTMENTS): " << sqlite().errmsg(db) << "\n";
            return false;
        }
        stmt.reset(raw);
    }
    if (sqlite().bind_int(stmt.get(), 1, patientId) != SQLITE_OK) { std::cerr << "bind_int failed (patientId)\n"; return false; }
    for (;;) {
        int s = sqlite().step(stmt.get());
        if (s == SQLITE_ROW) {
            int id = sqlite().column_int(stmt.get(), 0);
            const unsigned char* dd = sqlite().column_text(stmt.get(), 1);
            const unsigned char* nn = sqlite().column_text(stmt.get(), 2);
            out.emplace_back(id,
                std::string(reinterpret_cast<const char*>(dd ? dd : reinterpret_cast<const unsigned char*>(""))),
                std::string(reinterpret_cast<const char*>(nn ? nn : reinterpret_cast<const unsigned char*>(""))));
        } else if (s == SQLITE_DONE) {
            break;
        } else {
            std::cerr << "step failed (LIST_APPOINTMENTS): " << sqlite().errmsg(db) << "\n";
            return false;
        }
    }
    return true;
}

void sendUsage(int fd) {
    sendLine(fd, "OK Commands:");
    sendLine(fd, "  PING");
    sendLine(fd, "  ADD_PATIENT name;age");
    sendLine(fd, "  LIST_PATIENTS");
    sendLine(fd, "  GET_PATIENT id");
    sendLine(fd, "  ADD_APPOINTMENT patient_id;date;note");
    sendLine(fd, "  LIST_APPOINTMENTS patient_id");
    sendLine(fd, "  HELP");
    sendLine(fd, "  QUIT");
    sendLine(fd, "END");
}

void handleClient(int clientFd, sqlite3* db) {
    LineReader reader(clientFd);
    sendLine(clientFd, "WELCOME HospitalServer");

    std::string line;
    while (reader.readLine(line)) {
        line = trim(line);
        if (line.empty()) continue;
        if (line == "PING") {
            sendLine(clientFd, "PONG");
        } else if (line == "HELP") {
            sendUsage(clientFd);
        } else if (line == "LIST_PATIENTS") {
            std::vector<std::tuple<int,std::string,int>> rows;
            if (!cmdListPatients(db, rows)) {
                sendLine(clientFd, "ERR DB");
                continue;
            }
            for (const auto& r : rows) {
                std::ostringstream oss;
                oss << std::get<0>(r) << "|" << std::get<1>(r) << "|" << std::get<2>(r);
                sendLine(clientFd, oss.str());
            }
            sendLine(clientFd, "END");
        } else if (line.rfind("ADD_PATIENT", 0) == 0) {
            auto pos = line.find(' ');
            if (pos == std::string::npos) { sendLine(clientFd, "ERR Usage: ADD_PATIENT name;age"); continue; }
            std::string args = line.substr(pos + 1);
            auto parts = split(args, ';');
            if (parts.size() != 2) { sendLine(clientFd, "ERR Usage: ADD_PATIENT name;age"); continue; }
            std::string name = trim(parts[0]);
            int age = 0;
            try { age = std::stoi(trim(parts[1])); } catch (...) { sendLine(clientFd, "ERR age"); continue; }
            int id = 0;
            if (!cmdAddPatient(db, name, age, id)) { sendLine(clientFd, "ERR DB"); continue; }
            sendLine(clientFd, std::string("OK PATIENT_ID ") + std::to_string(id));
        } else if (line.rfind("GET_PATIENT", 0) == 0) {
            auto pos = line.find(' ');
            if (pos == std::string::npos) { sendLine(clientFd, "ERR Usage: GET_PATIENT id"); continue; }
            int id = 0; try { id = std::stoi(trim(line.substr(pos + 1))); } catch (...) { sendLine(clientFd, "ERR id"); continue; }
            std::tuple<int,std::string,int> row; bool found=false;
            if (!cmdGetPatient(db, id, row, found)) { sendLine(clientFd, "ERR DB"); continue; }
            if (!found) { sendLine(clientFd, "NOT_FOUND"); continue; }
            std::ostringstream oss;
            oss << std::get<0>(row) << "|" << std::get<1>(row) << "|" << std::get<2>(row);
            sendLine(clientFd, oss.str());
        } else if (line.rfind("ADD_APPOINTMENT", 0) == 0) {
            auto pos = line.find(' ');
            if (pos == std::string::npos) { sendLine(clientFd, "ERR Usage: ADD_APPOINTMENT patient_id;date;note"); continue; }
            std::string args = line.substr(pos + 1);
            auto parts = split(args, ';');
            if (parts.size() != 3) { sendLine(clientFd, "ERR Usage: ADD_APPOINTMENT patient_id;date;note"); continue; }
            int pid = 0; try { pid = std::stoi(trim(parts[0])); } catch (...) { sendLine(clientFd, "ERR patient_id"); continue; }
            std::string date = trim(parts[1]);
            std::string note = trim(parts[2]);
            int id = 0;
            if (!cmdAddAppointment(db, pid, date, note, id)) { sendLine(clientFd, "ERR DB"); continue; }
            sendLine(clientFd, std::string("OK APPOINTMENT_ID ") + std::to_string(id));
        } else if (line.rfind("LIST_APPOINTMENTS", 0) == 0) {
            auto pos = line.find(' ');
            if (pos == std::string::npos) { sendLine(clientFd, "ERR Usage: LIST_APPOINTMENTS patient_id"); continue; }
            int pid = 0; try { pid = std::stoi(trim(line.substr(pos + 1))); } catch (...) { sendLine(clientFd, "ERR patient_id"); continue; }
            std::vector<std::tuple<int,std::string,std::string>> rows;
            if (!cmdListAppointments(db, pid, rows)) { sendLine(clientFd, "ERR DB"); continue; }
            for (const auto& r : rows) {
                std::ostringstream oss;
                oss << std::get<0>(r) << "|" << std::get<1>(r) << "|" << std::get<2>(r);
                sendLine(clientFd, oss.str());
            }
            sendLine(clientFd, "END");
        } else if (line == "QUIT") {
            sendLine(clientFd, "BYE");
            break;
        } else {
            sendLine(clientFd, "ERR Unknown command");
        }
    }

    ::close(clientFd);
}

int createListenSocket(int port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) throw std::runtime_error("socket() failed: " + std::string(std::strerror(errno)));
    int opt = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd);
        throw std::runtime_error("bind() failed: " + std::string(std::strerror(errno)));
    }
    if (::listen(fd, 16) < 0) {
        ::close(fd);
        throw std::runtime_error("listen() failed: " + std::string(std::strerror(errno)));
    }
    return fd;
}

} // namespace

int main(int argc, char** argv) {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    ServerOptions opt = parseArgs(argc, argv);

    try {
        auto db = openDatabase(opt.dbPath);
        int listenFd = createListenSocket(opt.port);
        std::cout << "Hospital server listening on port " << opt.port << "\n";

        int served = 0;
        while (!g_shouldStop.load()) {
            sockaddr_in caddr{}; socklen_t clen = sizeof(caddr);
            int cfd = ::accept(listenFd, reinterpret_cast<sockaddr*>(&caddr), &clen);
            if (cfd < 0) {
                if (errno == EINTR) continue;
                std::cerr << "accept() error: " << std::strerror(errno) << "\n";
                continue;
            }

            char ip[64]{};
            std::cout << "Client connected: " << inet_ntop(AF_INET, &caddr.sin_addr, ip, sizeof(ip)) << ":" << ntohs(caddr.sin_port) << "\n";

            // For simplicity, share same db connection across threads (SQLite allows serialized threading by default)
            if (opt.once) {
                handleClient(cfd, db.get());
                served++;
                break;
            } else {
                std::thread th([cfd, &db]() { handleClient(cfd, db.get()); });
                th.detach();
                served++;
            }
        }

        ::close(listenFd);
        std::cout << "Server stopped\n";
    } catch (const std::exception& ex) {
        std::cerr << "Fatal: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
