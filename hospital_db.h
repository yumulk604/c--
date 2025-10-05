#ifndef HOSPITAL_DB_H
#define HOSPITAL_DB_H

#include <libpq-fe.h>
#include <string>
#include <vector>
#include <memory>

class HospitalDatabase {
private:
    PGconn* connection;
    std::string connection_string;
    
    bool execute_query(const std::string& query);
    PGresult* execute_query_with_result(const std::string& query);

public:
    HospitalDatabase();
    ~HospitalDatabase();
    
    bool connect(const std::string& host = "localhost", 
                 const std::string& port = "5432",
                 const std::string& dbname = "hospital_db",
                 const std::string& user = "postgres",
                 const std::string& password = "password");
    
    void disconnect();
    bool is_connected() const;
    
    // Database initialization
    bool create_tables();
    bool create_hypertables();
    bool insert_sample_data();
    
    // Patient operations
    uint32_t add_patient(const std::string& name, const std::string& surname,
                        const std::string& phone, const std::string& address,
                        uint8_t age, char gender, const std::string& blood_type);
    
    bool find_patient_by_id(uint32_t id, std::string& name, std::string& surname,
                           std::string& phone, std::string& address,
                           uint8_t& age, char& gender, std::string& blood_type,
                           uint32_t& registration_date);
    
    bool find_patient_by_name(const std::string& name, uint32_t& id,
                             std::string& surname, std::string& phone,
                             std::string& address, uint8_t& age, char& gender,
                             std::string& blood_type, uint32_t& registration_date);
    
    bool find_patient_by_phone(const std::string& phone, uint32_t& id,
                              std::string& name, std::string& surname,
                              std::string& address, uint8_t& age, char& gender,
                              std::string& blood_type, uint32_t& registration_date);
    
    // Doctor operations
    bool add_doctor(const std::string& name, const std::string& specialization);
    bool find_doctor_by_id(uint32_t id, std::string& name, std::string& specialization);
    std::vector<std::pair<uint32_t, std::string>> get_all_doctors();
    
    // Appointment operations
    bool book_appointment(uint32_t patient_id, uint32_t doctor_id, 
                         uint8_t day_of_week, uint8_t hour, const std::string& reason);
    bool is_appointment_available(uint32_t doctor_id, uint8_t day_of_week, uint8_t hour);
    std::vector<std::tuple<uint32_t, uint32_t, uint8_t, uint8_t, std::string, uint32_t>> 
        get_appointments_by_patient(uint32_t patient_id);
    
    // Medical record operations
    bool add_medical_record(uint32_t patient_id, uint32_t doctor_id,
                           const std::string& diagnosis, const std::string& treatment,
                           const std::string& medication, bool follow_up_required);
    
    std::vector<std::tuple<uint32_t, uint32_t, std::string, std::string, std::string, uint32_t, bool>>
        get_medical_records_by_patient(uint32_t patient_id);
    
    // Statistics
    uint32_t get_patient_count();
    uint32_t get_appointment_count();
    uint32_t get_medical_record_count();
};

#endif // HOSPITAL_DB_H