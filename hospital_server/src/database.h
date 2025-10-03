// Single header providing Database implementation.
// If HOSPITAL_USE_SQLITE is defined and sqlite3.h is available, use SQLite backend.
// Otherwise, fall back to a simple TSV file-backed implementation with no external deps.

#pragma once
#include <string>
#include <vector>
#include <tuple>
#include <stdexcept>
#include <mutex>
#include <fstream>
#include <sstream>
#include <algorithm>

class DatabaseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

#if defined(HOSPITAL_USE_SQLITE) && __has_include(<sqlite3.h>)
  #include <sqlite3.h>

class Database {
public:
    explicit Database(const std::string &path) {
        if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
            throw DatabaseError("Failed to open database: " + std::string(sqlite3_errmsg(db_)));
        }
        exec("PRAGMA foreign_keys = ON;");
        exec("CREATE TABLE IF NOT EXISTS patients (\n"
             "  id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
             "  name TEXT NOT NULL,\n"
             "  dob TEXT NOT NULL\n"
             ");");
        exec("CREATE TABLE IF NOT EXISTS appointments (\n"
             "  id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
             "  patient_id INTEGER NOT NULL,\n"
             "  when_ts TEXT NOT NULL,\n"
             "  notes TEXT NOT NULL,\n"
             "  FOREIGN KEY(patient_id) REFERENCES patients(id) ON DELETE CASCADE\n"
             ");");
    }

    ~Database() {
        if (db_) sqlite3_close(db_);
    }

    void exec(const std::string &sql) {
        char *errmsg = nullptr;
        if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
            std::string msg = errmsg ? errmsg : "unknown error";
            sqlite3_free(errmsg);
            throw DatabaseError("SQLite exec error: " + msg);
        }
    }

    int addPatient(const std::string &name, const std::string &dob) {
        sqlite3_stmt *stmt = nullptr;
        const char *sql = "INSERT INTO patients(name, dob) VALUES(?, ?);";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw DatabaseError("prepare addPatient: " + std::string(sqlite3_errmsg(db_)));
        }
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, dob.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            throw DatabaseError("step addPatient: " + std::string(sqlite3_errmsg(db_)));
        }
        int id = (int)sqlite3_last_insert_rowid(db_);
        sqlite3_finalize(stmt);
        return id;
    }

    std::vector<std::tuple<int, std::string, std::string>> listPatients() {
        std::vector<std::tuple<int, std::string, std::string>> result;
        const char *sql = "SELECT id, name, dob FROM patients ORDER BY id;";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw DatabaseError("prepare listPatients: " + std::string(sqlite3_errmsg(db_)));
        }
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char *name = sqlite3_column_text(stmt, 1);
            const unsigned char *dob = sqlite3_column_text(stmt, 2);
            result.emplace_back(id, name ? (const char*)name : "", dob ? (const char*)dob : "");
        }
        sqlite3_finalize(stmt);
        return result;
    }

    int addAppointment(int patientId, const std::string &whenTs, const std::string &notes) {
        sqlite3_stmt *stmt = nullptr;
        const char *sql = "INSERT INTO appointments(patient_id, when_ts, notes) VALUES(?, ?, ?);";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw DatabaseError("prepare addAppointment: " + std::string(sqlite3_errmsg(db_)));
        }
        sqlite3_bind_int(stmt, 1, patientId);
        sqlite3_bind_text(stmt, 2, whenTs.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, notes.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            throw DatabaseError("step addAppointment: " + std::string(sqlite3_errmsg(db_)));
        }
        int id = (int)sqlite3_last_insert_rowid(db_);
        sqlite3_finalize(stmt);
        return id;
    }

    std::vector<std::tuple<int, int, std::string, std::string>> listAppointments(int patientId) {
        std::vector<std::tuple<int, int, std::string, std::string>> result;
        const char *sql = "SELECT id, patient_id, when_ts, notes FROM appointments WHERE patient_id = ? ORDER BY when_ts;";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw DatabaseError("prepare listAppointments: " + std::string(sqlite3_errmsg(db_)));
        }
        sqlite3_bind_int(stmt, 1, patientId);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            int pid = sqlite3_column_int(stmt, 1);
            const unsigned char *when = sqlite3_column_text(stmt, 2);
            const unsigned char *notes = sqlite3_column_text(stmt, 3);
            result.emplace_back(id, pid, when ? (const char*)when : "", notes ? (const char*)notes : "");
        }
        sqlite3_finalize(stmt);
        return result;
    }

private:
    sqlite3 *db_ {nullptr};
};

#else // Fallback TSV-backed implementation

class Database {
public:
    explicit Database(const std::string &basePath)
        : patients_path_(basePath + ".patients.tsv"),
          appointments_path_(basePath + ".appointments.tsv") {
        loadPatients();
        loadAppointments();
    }

    int addPatient(const std::string &name, const std::string &dob) {
        std::lock_guard<std::mutex> lk(mu_);
        int id = next_patient_id_++;
        patients_.emplace_back(id, sanitize(name), sanitize(dob));
        appendLine(patients_path_, serialize({std::to_string(id), name, dob}));
        return id;
    }

    std::vector<std::tuple<int, std::string, std::string>> listPatients() {
        std::lock_guard<std::mutex> lk(mu_);
        return patients_;
    }

    int addAppointment(int patientId, const std::string &whenTs, const std::string &notes) {
        std::lock_guard<std::mutex> lk(mu_);
        // naive referential integrity: ensure patient exists
        auto it = std::find_if(patients_.begin(), patients_.end(), [&](const auto &t){ return std::get<0>(t) == patientId; });
        if (it == patients_.end()) {
            throw DatabaseError("patient not found");
        }
        int id = next_appt_id_++;
        appointments_.emplace_back(id, patientId, sanitize(whenTs), sanitize(notes));
        appendLine(appointments_path_, serialize({std::to_string(id), std::to_string(patientId), whenTs, notes}));
        return id;
    }

    std::vector<std::tuple<int, int, std::string, std::string>> listAppointments(int patientId) {
        std::lock_guard<std::mutex> lk(mu_);
        std::vector<std::tuple<int, int, std::string, std::string>> out;
        for (const auto &a : appointments_) {
            if (std::get<1>(a) == patientId) out.push_back(a);
        }
        return out;
    }

private:
    static std::string sanitize(const std::string &s) {
        std::string out = s;
        for (char &c : out) {
            if (c == '\t' || c == '\n' || c == '\r') c = ' ';
        }
        return out;
    }

    static std::string serialize(const std::vector<std::string> &fields) {
        std::ostringstream oss;
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i) oss << '\t';
            oss << sanitize(fields[i]);
        }
        return oss.str();
    }

    static std::vector<std::string> split_tsv(const std::string &line) {
        std::vector<std::string> out;
        std::string cur;
        for (char c : line) {
            if (c == '\t') { out.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        out.push_back(cur);
        return out;
    }

    static void appendLine(const std::string &path, const std::string &line) {
        std::ofstream f(path, std::ios::app);
        if (!f) throw DatabaseError("cannot open file for append: " + path);
        f << line << '\n';
    }

    void loadPatients() {
        std::ifstream f(patients_path_);
        if (!f) { next_patient_id_ = 1; return; }
        std::string line;
        int max_id = 0;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = split_tsv(line);
            if (fields.size() < 3) continue;
            int id = std::stoi(fields[0]);
            patients_.emplace_back(id, fields[1], fields[2]);
            if (id > max_id) max_id = id;
        }
        next_patient_id_ = max_id + 1;
    }

    void loadAppointments() {
        std::ifstream f(appointments_path_);
        if (!f) { next_appt_id_ = 1; return; }
        std::string line;
        int max_id = 0;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto fields = split_tsv(line);
            if (fields.size() < 4) continue;
            int id = std::stoi(fields[0]);
            int pid = std::stoi(fields[1]);
            appointments_.emplace_back(id, pid, fields[2], fields[3]);
            if (id > max_id) max_id = id;
        }
        next_appt_id_ = max_id + 1;
    }

    std::mutex mu_;
    std::string patients_path_;
    std::string appointments_path_;
    std::vector<std::tuple<int, std::string, std::string>> patients_;
    std::vector<std::tuple<int, int, std::string, std::string>> appointments_;
    int next_patient_id_ {1};
    int next_appt_id_ {1};
};

#endif
