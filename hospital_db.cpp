#include "hospital_db.h"
#include <iostream>
#include <sstream>
#include <ctime>

HospitalDatabase::HospitalDatabase() : connection(nullptr) {
    connection_string = "host=localhost port=5432 dbname=hospital_db user=postgres password=password";
}

HospitalDatabase::~HospitalDatabase() {
    disconnect();
}

bool HospitalDatabase::connect(const std::string& host, const std::string& port,
                              const std::string& dbname, const std::string& user,
                              const std::string& password) {
    std::ostringstream conn_str;
    conn_str << "host=" << host << " port=" << port << " dbname=" << dbname 
             << " user=" << user << " password=" << password;
    
    connection_string = conn_str.str();
    connection = PQconnectdb(connection_string.c_str());
    
    if (PQstatus(connection) != CONNECTION_OK) {
        std::cerr << "Connection to database failed: " << PQerrorMessage(connection) << std::endl;
        PQfinish(connection);
        connection = nullptr;
        return false;
    }
    
    std::cout << "Connected to TimescaleDB successfully" << std::endl;
    return true;
}

void HospitalDatabase::disconnect() {
    if (connection) {
        PQfinish(connection);
        connection = nullptr;
    }
}

bool HospitalDatabase::is_connected() const {
    return connection && PQstatus(connection) == CONNECTION_OK;
}

bool HospitalDatabase::execute_query(const std::string& query) {
    if (!is_connected()) return false;
    
    PGresult* result = PQexec(connection, query.c_str());
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        std::cerr << "Query failed: " << PQerrorMessage(connection) << std::endl;
        PQclear(result);
        return false;
    }
    
    PQclear(result);
    return true;
}

PGresult* HospitalDatabase::execute_query_with_result(const std::string& query) {
    if (!is_connected()) return nullptr;
    
    PGresult* result = PQexec(connection, query.c_str());
    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        std::cerr << "Query failed: " << PQerrorMessage(connection) << std::endl;
        PQclear(result);
        return nullptr;
    }
    
    return result;
}

bool HospitalDatabase::create_tables() {
    std::cout << "Creating database tables..." << std::endl;
    
    // Create patients table
    std::string create_patients = R"(
        CREATE TABLE IF NOT EXISTS patients (
            id SERIAL PRIMARY KEY,
            name VARCHAR(64) NOT NULL,
            surname VARCHAR(64) NOT NULL,
            phone VARCHAR(16) UNIQUE NOT NULL,
            address VARCHAR(128),
            age SMALLINT NOT NULL,
            gender CHAR(1) CHECK (gender IN ('M', 'F')),
            blood_type VARCHAR(4),
            registration_date TIMESTAMPTZ DEFAULT NOW()
        );
    )";
    
    if (!execute_query(create_patients)) return false;
    
    // Create doctors table
    std::string create_doctors = R"(
        CREATE TABLE IF NOT EXISTS doctors (
            id SERIAL PRIMARY KEY,
            name VARCHAR(64) NOT NULL,
            specialization VARCHAR(32) NOT NULL,
            created_at TIMESTAMPTZ DEFAULT NOW()
        );
    )";
    
    if (!execute_query(create_doctors)) return false;
    
    // Create appointments table
    std::string create_appointments = R"(
        CREATE TABLE IF NOT EXISTS appointments (
            id SERIAL PRIMARY KEY,
            patient_id INTEGER REFERENCES patients(id) ON DELETE CASCADE,
            doctor_id INTEGER REFERENCES doctors(id) ON DELETE CASCADE,
            day_of_week SMALLINT CHECK (day_of_week >= 0 AND day_of_week <= 6),
            hour SMALLINT CHECK (hour >= 0 AND hour <= 23),
            reason VARCHAR(128),
            appointment_date TIMESTAMPTZ DEFAULT NOW(),
            UNIQUE(doctor_id, day_of_week, hour)
        );
    )";
    
    if (!execute_query(create_appointments)) return false;
    
    // Create medical_records table
    std::string create_medical_records = R"(
        CREATE TABLE IF NOT EXISTS medical_records (
            id SERIAL PRIMARY KEY,
            patient_id INTEGER REFERENCES patients(id) ON DELETE CASCADE,
            doctor_id INTEGER REFERENCES doctors(id) ON DELETE CASCADE,
            diagnosis TEXT,
            treatment TEXT,
            medication VARCHAR(128),
            visit_date TIMESTAMPTZ DEFAULT NOW(),
            follow_up_required BOOLEAN DEFAULT FALSE
        );
    )";
    
    if (!execute_query(create_medical_records)) return false;
    
    std::cout << "Database tables created successfully" << std::endl;
    return true;
}

bool HospitalDatabase::create_hypertables() {
    std::cout << "Creating TimescaleDB hypertables..." << std::endl;
    
    // Convert appointments to hypertable
    std::string create_appointments_hypertable = R"(
        SELECT create_hypertable('appointments', 'appointment_date', 
                                chunk_time_interval => INTERVAL '1 day');
    )";
    
    execute_query(create_appointments_hypertable); // Ignore error if already exists
    
    // Convert medical_records to hypertable
    std::string create_medical_records_hypertable = R"(
        SELECT create_hypertable('medical_records', 'visit_date', 
                                chunk_time_interval => INTERVAL '1 day');
    )";
    
    execute_query(create_medical_records_hypertable); // Ignore error if already exists
    
    std::cout << "TimescaleDB hypertables created successfully" << std::endl;
    return true;
}

bool HospitalDatabase::insert_sample_data() {
    std::cout << "Inserting sample data..." << std::endl;
    
    // Insert sample doctors
    std::string insert_doctors = R"(
        INSERT INTO doctors (name, specialization) VALUES
        ('Dr. Smith', 'Cardiology'),
        ('Dr. Johnson', 'Neurology'),
        ('Dr. Williams', 'Pediatrics'),
        ('Dr. Brown', 'Orthopedics')
        ON CONFLICT DO NOTHING;
    )";
    
    if (!execute_query(insert_doctors)) return false;
    
    std::cout << "Sample data inserted successfully" << std::endl;
    return true;
}

uint32_t HospitalDatabase::add_patient(const std::string& name, const std::string& surname,
                                      const std::string& phone, const std::string& address,
                                      uint8_t age, char gender, const std::string& blood_type) {
    std::ostringstream query;
    query << "INSERT INTO patients (name, surname, phone, address, age, gender, blood_type) "
          << "VALUES ('" << name << "', '" << surname << "', '" << phone << "', '"
          << address << "', " << static_cast<int>(age) << ", '" << gender << "', '"
          << blood_type << "') RETURNING id;";
    
    PGresult* result = execute_query_with_result(query.str());
    if (!result) return 0;
    
    uint32_t patient_id = static_cast<uint32_t>(std::stoul(PQgetvalue(result, 0, 0)));
    PQclear(result);
    return patient_id;
}

bool HospitalDatabase::find_patient_by_id(uint32_t id, std::string& name, std::string& surname,
                                        std::string& phone, std::string& address,
                                        uint8_t& age, char& gender, std::string& blood_type,
                                        uint32_t& registration_date) {
    std::ostringstream query;
    query << "SELECT name, surname, phone, address, age, gender, blood_type, "
          << "EXTRACT(EPOCH FROM registration_date)::INTEGER "
          << "FROM patients WHERE id = " << id << ";";
    
    PGresult* result = execute_query_with_result(query.str());
    if (!result || PQntuples(result) == 0) {
        if (result) PQclear(result);
        return false;
    }
    
    name = PQgetvalue(result, 0, 0);
    surname = PQgetvalue(result, 0, 1);
    phone = PQgetvalue(result, 0, 2);
    address = PQgetvalue(result, 0, 3);
    age = static_cast<uint8_t>(std::stoi(PQgetvalue(result, 0, 4)));
    gender = PQgetvalue(result, 0, 5)[0];
    blood_type = PQgetvalue(result, 0, 6);
    registration_date = static_cast<uint32_t>(std::stoul(PQgetvalue(result, 0, 7)));
    
    PQclear(result);
    return true;
}

bool HospitalDatabase::find_patient_by_name(const std::string& name, uint32_t& id,
                                           std::string& surname, std::string& phone,
                                           std::string& address, uint8_t& age, char& gender,
                                           std::string& blood_type, uint32_t& registration_date) {
    std::ostringstream query;
    query << "SELECT id, surname, phone, address, age, gender, blood_type, "
          << "EXTRACT(EPOCH FROM registration_date)::INTEGER "
          << "FROM patients WHERE name ILIKE '%" << name << "%' OR surname ILIKE '%" << name << "%';";
    
    PGresult* result = execute_query_with_result(query.str());
    if (!result || PQntuples(result) == 0) {
        if (result) PQclear(result);
        return false;
    }
    
    id = static_cast<uint32_t>(std::stoul(PQgetvalue(result, 0, 0)));
    surname = PQgetvalue(result, 0, 1);
    phone = PQgetvalue(result, 0, 2);
    address = PQgetvalue(result, 0, 3);
    age = static_cast<uint8_t>(std::stoi(PQgetvalue(result, 0, 4)));
    gender = PQgetvalue(result, 0, 5)[0];
    blood_type = PQgetvalue(result, 0, 6);
    registration_date = static_cast<uint32_t>(std::stoul(PQgetvalue(result, 0, 7)));
    
    PQclear(result);
    return true;
}

bool HospitalDatabase::find_patient_by_phone(const std::string& phone, uint32_t& id,
                                            std::string& name, std::string& surname,
                                            std::string& address, uint8_t& age, char& gender,
                                            std::string& blood_type, uint32_t& registration_date) {
    std::ostringstream query;
    query << "SELECT id, name, surname, address, age, gender, blood_type, "
          << "EXTRACT(EPOCH FROM registration_date)::INTEGER "
          << "FROM patients WHERE phone = '" << phone << "';";
    
    PGresult* result = execute_query_with_result(query.str());
    if (!result || PQntuples(result) == 0) {
        if (result) PQclear(result);
        return false;
    }
    
    id = static_cast<uint32_t>(std::stoul(PQgetvalue(result, 0, 0)));
    name = PQgetvalue(result, 0, 1);
    surname = PQgetvalue(result, 0, 2);
    address = PQgetvalue(result, 0, 3);
    age = static_cast<uint8_t>(std::stoi(PQgetvalue(result, 0, 4)));
    gender = PQgetvalue(result, 0, 5)[0];
    blood_type = PQgetvalue(result, 0, 6);
    registration_date = static_cast<uint32_t>(std::stoul(PQgetvalue(result, 0, 7)));
    
    PQclear(result);
    return true;
}

bool HospitalDatabase::add_doctor(const std::string& name, const std::string& specialization) {
    std::ostringstream query;
    query << "INSERT INTO doctors (name, specialization) VALUES ('" 
          << name << "', '" << specialization << "');";
    
    return execute_query(query.str());
}

bool HospitalDatabase::find_doctor_by_id(uint32_t id, std::string& name, std::string& specialization) {
    std::ostringstream query;
    query << "SELECT name, specialization FROM doctors WHERE id = " << id << ";";
    
    PGresult* result = execute_query_with_result(query.str());
    if (!result || PQntuples(result) == 0) {
        if (result) PQclear(result);
        return false;
    }
    
    name = PQgetvalue(result, 0, 0);
    specialization = PQgetvalue(result, 0, 1);
    
    PQclear(result);
    return true;
}

std::vector<std::pair<uint32_t, std::string>> HospitalDatabase::get_all_doctors() {
    std::vector<std::pair<uint32_t, std::string>> doctors;
    
    std::string query = "SELECT id, name, specialization FROM doctors ORDER BY id;";
    PGresult* result = execute_query_with_result(query);
    
    if (result && PQntuples(result) > 0) {
        for (int i = 0; i < PQntuples(result); i++) {
            uint32_t id = static_cast<uint32_t>(std::stoul(PQgetvalue(result, i, 0)));
            std::string name_spec = std::string(PQgetvalue(result, i, 1)) + " (" + 
                                   PQgetvalue(result, i, 2) + ")";
            doctors.push_back({id, name_spec});
        }
    }
    
    if (result) PQclear(result);
    return doctors;
}

bool HospitalDatabase::book_appointment(uint32_t patient_id, uint32_t doctor_id, 
                                      uint8_t day_of_week, uint8_t hour, const std::string& reason) {
    std::ostringstream query;
    query << "INSERT INTO appointments (patient_id, doctor_id, day_of_week, hour, reason) "
          << "VALUES (" << patient_id << ", " << doctor_id << ", " 
          << static_cast<int>(day_of_week) << ", " << static_cast<int>(hour) 
          << ", '" << reason << "');";
    
    return execute_query(query.str());
}

bool HospitalDatabase::is_appointment_available(uint32_t doctor_id, uint8_t day_of_week, uint8_t hour) {
    std::ostringstream query;
    query << "SELECT COUNT(*) FROM appointments WHERE doctor_id = " << doctor_id 
          << " AND day_of_week = " << static_cast<int>(day_of_week) 
          << " AND hour = " << static_cast<int>(hour) << ";";
    
    PGresult* result = execute_query_with_result(query.str());
    if (!result) return false;
    
    int count = std::stoi(PQgetvalue(result, 0, 0));
    PQclear(result);
    
    return count == 0;
}

bool HospitalDatabase::add_medical_record(uint32_t patient_id, uint32_t doctor_id,
                                          const std::string& diagnosis, const std::string& treatment,
                                          const std::string& medication, bool follow_up_required) {
    std::ostringstream query;
    query << "INSERT INTO medical_records (patient_id, doctor_id, diagnosis, treatment, medication, follow_up_required) "
          << "VALUES (" << patient_id << ", " << doctor_id << ", '" << diagnosis 
          << "', '" << treatment << "', '" << medication << "', " 
          << (follow_up_required ? "TRUE" : "FALSE") << ");";
    
    return execute_query(query.str());
}

uint32_t HospitalDatabase::get_patient_count() {
    std::string query = "SELECT COUNT(*) FROM patients;";
    PGresult* result = execute_query_with_result(query);
    
    if (!result) return 0;
    
    uint32_t count = static_cast<uint32_t>(std::stoul(PQgetvalue(result, 0, 0)));
    PQclear(result);
    
    return count;
}

uint32_t HospitalDatabase::get_appointment_count() {
    std::string query = "SELECT COUNT(*) FROM appointments;";
    PGresult* result = execute_query_with_result(query);
    
    if (!result) return 0;
    
    uint32_t count = static_cast<uint32_t>(std::stoul(PQgetvalue(result, 0, 0)));
    PQclear(result);
    
    return count;
}

uint32_t HospitalDatabase::get_medical_record_count() {
    std::string query = "SELECT COUNT(*) FROM medical_records;";
    PGresult* result = execute_query_with_result(query);
    
    if (!result) return 0;
    
    uint32_t count = static_cast<uint32_t>(std::stoul(PQgetvalue(result, 0, 0)));
    PQclear(result);
    
    return count;
}