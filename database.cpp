#include "database.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>

Database::Database() : connection(nullptr), port(3306) {
    mysql_library_init(0, nullptr, nullptr);
}

Database::~Database() {
    Disconnect();
    mysql_library_end();
}

bool Database::Connect(const std::string& host, const std::string& username, 
                      const std::string& password, const std::string& database_name, 
                      unsigned int port) {
    this->host = host;
    this->username = username;
    this->password = password;
    this->database_name = database_name;
    this->port = port;
    
    connection = mysql_init(nullptr);
    if (!connection) {
        std::cerr << "Failed to initialize MySQL connection" << std::endl;
        return false;
    }
    
    // Set connection options
    bool reconnect = true;
    mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect);
    
    // Connect to database
    if (!mysql_real_connect(connection, host.c_str(), username.c_str(), 
                           password.c_str(), database_name.c_str(), port, nullptr, 0)) {
        LogError("Connection");
        mysql_close(connection);
        connection = nullptr;
        return false;
    }
    
    // Set charset to UTF-8
    if (mysql_set_character_set(connection, "utf8")) {
        LogError("Setting charset");
    }
    
    std::cout << "Successfully connected to database: " << database_name << std::endl;
    return true;
}

void Database::Disconnect() {
    if (connection) {
        mysql_close(connection);
        connection = nullptr;
        std::cout << "Disconnected from database" << std::endl;
    }
}

bool Database::IsConnected() const {
    return connection != nullptr && mysql_ping(connection) == 0;
}

bool Database::AuthenticateUser(const std::string& username, const std::string& password, 
                               uint8_t userType, uint32_t& userID) {
    if (!IsConnected()) return false;
    
    std::string query = "SELECT user_id FROM users WHERE username = '" + 
                       EscapeString(username) + "' AND password_hash = SHA2('" + 
                       EscapeString(password) + "', 256) AND user_type = " + 
                       std::to_string(userType) + " AND is_active = TRUE";
    
    MYSQL_RES* result = ExecuteSelectQuery(query);
    if (!result) return false;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        userID = static_cast<uint32_t>(std::stoul(row[0]));
        mysql_free_result(result);
        return true;
    }
    
    mysql_free_result(result);
    return false;
}

bool Database::RegisterPatient(const TPacketCGRegisterPatient& packet, uint32_t& patientID) {
    if (!IsConnected()) return false;
    
    // First, create a user account
    std::string userQuery = "INSERT INTO users (username, password_hash, user_type) VALUES ('" +
                           EscapeString(packet.szEmail) + "', SHA2('temp123', 256), 1)";
    
    if (!ExecuteQuery(userQuery)) return false;
    
    uint32_t userID = static_cast<uint32_t>(mysql_insert_id(connection));
    
    // Then create the patient record
    std::string patientQuery = "INSERT INTO patients (user_id, first_name, last_name, date_of_birth, "
                              "gender, phone, email, address, emergency_contact, emergency_phone) VALUES (" +
                              std::to_string(userID) + ", '" +
                              EscapeString(packet.szFirstName) + "', '" +
                              EscapeString(packet.szLastName) + "', '" +
                              EscapeString(packet.szDateOfBirth) + "', '" +
                              EscapeString(packet.szGender) + "', '" +
                              EscapeString(packet.szPhone) + "', '" +
                              EscapeString(packet.szEmail) + "', '" +
                              EscapeString(packet.szAddress) + "', '" +
                              EscapeString(packet.szEmergencyContact) + "', '" +
                              EscapeString(packet.szEmergencyPhone) + "')";
    
    if (!ExecuteQuery(patientQuery)) return false;
    
    patientID = static_cast<uint32_t>(mysql_insert_id(connection));
    return true;
}

bool Database::GetPatientInfo(uint32_t patientID, TPatientInfo& patientInfo) {
    if (!IsConnected()) return false;
    
    std::string query = "SELECT patient_id, first_name, last_name, date_of_birth, gender, "
                       "phone, email, address, emergency_contact, emergency_phone, registration_date "
                       "FROM patients WHERE patient_id = " + std::to_string(patientID);
    
    MYSQL_RES* result = ExecuteSelectQuery(query);
    if (!result) return false;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        patientInfo.dwPatientID = static_cast<uint32_t>(std::stoul(row[0]));
        SafeStrCopy(patientInfo.szFirstName, row[1] ? row[1] : "", sizeof(patientInfo.szFirstName));
        SafeStrCopy(patientInfo.szLastName, row[2] ? row[2] : "", sizeof(patientInfo.szLastName));
        SafeStrCopy(patientInfo.szDateOfBirth, row[3] ? row[3] : "", sizeof(patientInfo.szDateOfBirth));
        SafeStrCopy(patientInfo.szGender, row[4] ? row[4] : "", sizeof(patientInfo.szGender));
        SafeStrCopy(patientInfo.szPhone, row[5] ? row[5] : "", sizeof(patientInfo.szPhone));
        SafeStrCopy(patientInfo.szEmail, row[6] ? row[6] : "", sizeof(patientInfo.szEmail));
        SafeStrCopy(patientInfo.szAddress, row[7] ? row[7] : "", sizeof(patientInfo.szAddress));
        SafeStrCopy(patientInfo.szEmergencyContact, row[8] ? row[8] : "", sizeof(patientInfo.szEmergencyContact));
        SafeStrCopy(patientInfo.szEmergencyPhone, row[9] ? row[9] : "", sizeof(patientInfo.szEmergencyPhone));
        SafeStrCopy(patientInfo.szRegistrationDate, row[10] ? row[10] : "", sizeof(patientInfo.szRegistrationDate));
        
        mysql_free_result(result);
        return true;
    }
    
    mysql_free_result(result);
    return false;
}

bool Database::UpdatePatientInfo(const TPatientInfo& patientInfo) {
    if (!IsConnected()) return false;
    
    std::string query = "UPDATE patients SET "
                       "first_name = '" + EscapeString(patientInfo.szFirstName) + "', "
                       "last_name = '" + EscapeString(patientInfo.szLastName) + "', "
                       "date_of_birth = '" + EscapeString(patientInfo.szDateOfBirth) + "', "
                       "gender = '" + EscapeString(patientInfo.szGender) + "', "
                       "phone = '" + EscapeString(patientInfo.szPhone) + "', "
                       "email = '" + EscapeString(patientInfo.szEmail) + "', "
                       "address = '" + EscapeString(patientInfo.szAddress) + "', "
                       "emergency_contact = '" + EscapeString(patientInfo.szEmergencyContact) + "', "
                       "emergency_phone = '" + EscapeString(patientInfo.szEmergencyPhone) + "' "
                       "WHERE patient_id = " + std::to_string(patientInfo.dwPatientID);
    
    return ExecuteQuery(query);
}

bool Database::GetDoctors(const std::string& specialization, std::vector<TDoctorInfo>& doctors) {
    if (!IsConnected()) return false;
    
    std::string query = "SELECT doctor_id, first_name, last_name, specialization, phone, "
                       "email, office_room, is_available FROM doctors WHERE is_available = TRUE";
    
    if (!specialization.empty()) {
        query += " AND specialization = '" + EscapeString(specialization) + "'";
    }
    
    query += " ORDER BY last_name, first_name";
    
    MYSQL_RES* result = ExecuteSelectQuery(query);
    if (!result) return false;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        TDoctorInfo doctor;
        doctor.dwDoctorID = static_cast<uint32_t>(std::stoul(row[0]));
        SafeStrCopy(doctor.szFirstName, row[1] ? row[1] : "", sizeof(doctor.szFirstName));
        SafeStrCopy(doctor.szLastName, row[2] ? row[2] : "", sizeof(doctor.szLastName));
        SafeStrCopy(doctor.szSpecialization, row[3] ? row[3] : "", sizeof(doctor.szSpecialization));
        SafeStrCopy(doctor.szPhone, row[4] ? row[4] : "", sizeof(doctor.szPhone));
        SafeStrCopy(doctor.szEmail, row[5] ? row[5] : "", sizeof(doctor.szEmail));
        SafeStrCopy(doctor.szOfficeRoom, row[6] ? row[6] : "", sizeof(doctor.szOfficeRoom));
        doctor.bAvailable = row[7] && std::string(row[7]) == "1" ? 1 : 0;
        
        doctors.push_back(doctor);
    }
    
    mysql_free_result(result);
    return true;
}

bool Database::GetDoctorInfo(uint32_t doctorID, TDoctorInfo& doctorInfo) {
    if (!IsConnected()) return false;
    
    std::string query = "SELECT doctor_id, first_name, last_name, specialization, phone, "
                       "email, office_room, is_available FROM doctors WHERE doctor_id = " + 
                       std::to_string(doctorID);
    
    MYSQL_RES* result = ExecuteSelectQuery(query);
    if (!result) return false;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        doctorInfo.dwDoctorID = static_cast<uint32_t>(std::stoul(row[0]));
        SafeStrCopy(doctorInfo.szFirstName, row[1] ? row[1] : "", sizeof(doctorInfo.szFirstName));
        SafeStrCopy(doctorInfo.szLastName, row[2] ? row[2] : "", sizeof(doctorInfo.szLastName));
        SafeStrCopy(doctorInfo.szSpecialization, row[3] ? row[3] : "", sizeof(doctorInfo.szSpecialization));
        SafeStrCopy(doctorInfo.szPhone, row[4] ? row[4] : "", sizeof(doctorInfo.szPhone));
        SafeStrCopy(doctorInfo.szEmail, row[5] ? row[5] : "", sizeof(doctorInfo.szEmail));
        SafeStrCopy(doctorInfo.szOfficeRoom, row[6] ? row[6] : "", sizeof(doctorInfo.szOfficeRoom));
        doctorInfo.bAvailable = row[7] && std::string(row[7]) == "1" ? 1 : 0;
        
        mysql_free_result(result);
        return true;
    }
    
    mysql_free_result(result);
    return false;
}

bool Database::BookAppointment(const TPacketCGBookAppointment& packet, uint32_t& appointmentID) {
    if (!IsConnected()) return false;
    
    // Check for conflicts first
    if (CheckAppointmentConflict(packet.dwDoctorID, packet.szAppointmentDate, packet.szAppointmentTime)) {
        return false;
    }
    
    std::string query = "INSERT INTO appointments (patient_id, doctor_id, appointment_date, "
                       "appointment_time, reason, status) VALUES (" +
                       std::to_string(packet.dwPatientID) + ", " +
                       std::to_string(packet.dwDoctorID) + ", '" +
                       EscapeString(packet.szAppointmentDate) + "', '" +
                       EscapeString(packet.szAppointmentTime) + "', '" +
                       EscapeString(packet.szReason) + "', 'SCHEDULED')";
    
    if (!ExecuteQuery(query)) return false;
    
    appointmentID = static_cast<uint32_t>(mysql_insert_id(connection));
    return true;
}

bool Database::GetAppointments(uint32_t patientID, const std::string& startDate, 
                              const std::string& endDate, std::vector<TAppointmentInfo>& appointments) {
    if (!IsConnected()) return false;
    
    std::string query = "SELECT a.appointment_id, a.patient_id, a.doctor_id, "
                       "CONCAT(p.first_name, ' ', p.last_name) as patient_name, "
                       "CONCAT(d.first_name, ' ', d.last_name) as doctor_name, "
                       "a.appointment_date, a.appointment_time, a.reason, a.status "
                       "FROM appointments a "
                       "JOIN patients p ON a.patient_id = p.patient_id "
                       "JOIN doctors d ON a.doctor_id = d.doctor_id "
                       "WHERE a.patient_id = " + std::to_string(patientID);
    
    if (!startDate.empty()) {
        query += " AND a.appointment_date >= '" + EscapeString(startDate) + "'";
    }
    if (!endDate.empty()) {
        query += " AND a.appointment_date <= '" + EscapeString(endDate) + "'";
    }
    
    query += " ORDER BY a.appointment_date, a.appointment_time";
    
    MYSQL_RES* result = ExecuteSelectQuery(query);
    if (!result) return false;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        TAppointmentInfo appointment;
        appointment.dwAppointmentID = static_cast<uint32_t>(std::stoul(row[0]));
        appointment.dwPatientID = static_cast<uint32_t>(std::stoul(row[1]));
        appointment.dwDoctorID = static_cast<uint32_t>(std::stoul(row[2]));
        SafeStrCopy(appointment.szPatientName, row[3] ? row[3] : "", sizeof(appointment.szPatientName));
        SafeStrCopy(appointment.szDoctorName, row[4] ? row[4] : "", sizeof(appointment.szDoctorName));
        SafeStrCopy(appointment.szAppointmentDate, row[5] ? row[5] : "", sizeof(appointment.szAppointmentDate));
        SafeStrCopy(appointment.szAppointmentTime, row[6] ? row[6] : "", sizeof(appointment.szAppointmentTime));
        SafeStrCopy(appointment.szReason, row[7] ? row[7] : "", sizeof(appointment.szReason));
        SafeStrCopy(appointment.szStatus, row[8] ? row[8] : "", sizeof(appointment.szStatus));
        
        appointments.push_back(appointment);
    }
    
    mysql_free_result(result);
    return true;
}

bool Database::UpdateAppointmentStatus(uint32_t appointmentID, const std::string& status) {
    if (!IsConnected()) return false;
    
    std::string query = "UPDATE appointments SET status = '" + EscapeString(status) + 
                       "' WHERE appointment_id = " + std::to_string(appointmentID);
    
    return ExecuteQuery(query);
}

bool Database::CheckAppointmentConflict(uint32_t doctorID, const std::string& date, const std::string& time) {
    if (!IsConnected()) return true; // Assume conflict if can't check
    
    std::string query = "SELECT COUNT(*) FROM appointments WHERE doctor_id = " + 
                       std::to_string(doctorID) + " AND appointment_date = '" + 
                       EscapeString(date) + "' AND appointment_time = '" + 
                       EscapeString(time) + "' AND status = 'SCHEDULED'";
    
    MYSQL_RES* result = ExecuteSelectQuery(query);
    if (!result) return true;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    bool conflict = false;
    if (row && std::stoi(row[0]) > 0) {
        conflict = true;
    }
    
    mysql_free_result(result);
    return conflict;
}

bool Database::AddMedicalRecord(const TPacketCGAddMedicalRecord& packet, uint32_t& recordID) {
    if (!IsConnected()) return false;
    
    std::string query = "INSERT INTO medical_records (patient_id, doctor_id, diagnosis, "
                       "treatment, prescription, notes) VALUES (" +
                       std::to_string(packet.dwPatientID) + ", " +
                       std::to_string(packet.dwDoctorID) + ", '" +
                       EscapeString(packet.szDiagnosis) + "', '" +
                       EscapeString(packet.szTreatment) + "', '" +
                       EscapeString(packet.szPrescription) + "', '" +
                       EscapeString(packet.szNotes) + "')";
    
    if (!ExecuteQuery(query)) return false;
    
    recordID = static_cast<uint32_t>(mysql_insert_id(connection));
    return true;
}

bool Database::GetMedicalHistory(uint32_t patientID, const std::string& startDate, 
                                const std::string& endDate, std::vector<TMedicalRecord>& records) {
    if (!IsConnected()) return false;
    
    std::string query = "SELECT mr.record_id, mr.patient_id, mr.doctor_id, "
                       "CONCAT(d.first_name, ' ', d.last_name) as doctor_name, "
                       "mr.record_date, mr.diagnosis, mr.treatment, mr.prescription, mr.notes "
                       "FROM medical_records mr "
                       "JOIN doctors d ON mr.doctor_id = d.doctor_id "
                       "WHERE mr.patient_id = " + std::to_string(patientID);
    
    if (!startDate.empty()) {
        query += " AND DATE(mr.record_date) >= '" + EscapeString(startDate) + "'";
    }
    if (!endDate.empty()) {
        query += " AND DATE(mr.record_date) <= '" + EscapeString(endDate) + "'";
    }
    
    query += " ORDER BY mr.record_date DESC";
    
    MYSQL_RES* result = ExecuteSelectQuery(query);
    if (!result) return false;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        TMedicalRecord record;
        record.dwRecordID = static_cast<uint32_t>(std::stoul(row[0]));
        record.dwPatientID = static_cast<uint32_t>(std::stoul(row[1]));
        record.dwDoctorID = static_cast<uint32_t>(std::stoul(row[2]));
        SafeStrCopy(record.szDoctorName, row[3] ? row[3] : "", sizeof(record.szDoctorName));
        SafeStrCopy(record.szRecordDate, row[4] ? row[4] : "", sizeof(record.szRecordDate));
        SafeStrCopy(record.szDiagnosis, row[5] ? row[5] : "", sizeof(record.szDiagnosis));
        SafeStrCopy(record.szTreatment, row[6] ? row[6] : "", sizeof(record.szTreatment));
        SafeStrCopy(record.szPrescription, row[7] ? row[7] : "", sizeof(record.szPrescription));
        SafeStrCopy(record.szNotes, row[8] ? row[8] : "", sizeof(record.szNotes));
        
        records.push_back(record);
    }
    
    mysql_free_result(result);
    return true;
}

bool Database::ExecuteQuery(const std::string& query) {
    if (!IsConnected()) return false;
    
    if (mysql_query(connection, query.c_str())) {
        LogError("Query execution: " + query);
        return false;
    }
    
    return true;
}

MYSQL_RES* Database::ExecuteSelectQuery(const std::string& query) {
    if (!ExecuteQuery(query)) return nullptr;
    
    MYSQL_RES* result = mysql_store_result(connection);
    if (!result && mysql_field_count(connection) > 0) {
        LogError("Storing result");
        return nullptr;
    }
    
    return result;
}

std::string Database::EscapeString(const std::string& input) {
    if (!connection) return input;
    
    char* escaped = new char[input.length() * 2 + 1];
    mysql_real_escape_string(connection, escaped, input.c_str(), input.length());
    std::string result(escaped);
    delete[] escaped;
    
    return result;
}

void Database::LogError(const std::string& operation) {
    if (connection) {
        std::cerr << "MySQL Error in " << operation << ": " 
                  << mysql_error(connection) << " (Error code: " 
                  << mysql_errno(connection) << ")" << std::endl;
    } else {
        std::cerr << "MySQL Error in " << operation << ": No connection" << std::endl;
    }
}

std::string Database::GetCurrentDateTime() {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}