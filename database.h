#ifndef DATABASE_H
#define DATABASE_H

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include "hospital_packet.h"

class Database {
private:
    MYSQL* connection;
    std::string host;
    std::string username;
    std::string password;
    std::string database_name;
    unsigned int port;

public:
    Database();
    ~Database();
    
    bool Connect(const std::string& host, const std::string& username, 
                const std::string& password, const std::string& database_name, 
                unsigned int port = 3306);
    void Disconnect();
    bool IsConnected() const;
    
    // User authentication
    bool AuthenticateUser(const std::string& username, const std::string& password, 
                         uint8_t userType, uint32_t& userID);
    
    // Patient management
    bool RegisterPatient(const TPacketCGRegisterPatient& packet, uint32_t& patientID);
    bool GetPatientInfo(uint32_t patientID, TPatientInfo& patientInfo);
    bool UpdatePatientInfo(const TPatientInfo& patientInfo);
    
    // Doctor management
    bool GetDoctors(const std::string& specialization, std::vector<TDoctorInfo>& doctors);
    bool GetDoctorInfo(uint32_t doctorID, TDoctorInfo& doctorInfo);
    
    // Appointment management
    bool BookAppointment(const TPacketCGBookAppointment& packet, uint32_t& appointmentID);
    bool GetAppointments(uint32_t patientID, const std::string& startDate, 
                        const std::string& endDate, std::vector<TAppointmentInfo>& appointments);
    bool UpdateAppointmentStatus(uint32_t appointmentID, const std::string& status);
    bool CheckAppointmentConflict(uint32_t doctorID, const std::string& date, const std::string& time);
    
    // Medical records
    bool AddMedicalRecord(const TPacketCGAddMedicalRecord& packet, uint32_t& recordID);
    bool GetMedicalHistory(uint32_t patientID, const std::string& startDate, 
                          const std::string& endDate, std::vector<TMedicalRecord>& records);
    
    // Utility functions
    bool ExecuteQuery(const std::string& query);
    MYSQL_RES* ExecuteSelectQuery(const std::string& query);
    std::string EscapeString(const std::string& input);
    
private:
    void LogError(const std::string& operation);
    std::string GetCurrentDateTime();
};

#endif // DATABASE_H