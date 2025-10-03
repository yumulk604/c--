#ifndef HOSPITAL_PACKET_H
#define HOSPITAL_PACKET_H

#include <cstdint>
#include <cstring>

// Packet headers for hospital operations
enum HospitalPacketHeader : uint8_t {
    HEADER_PATIENT_REGISTER = 0x01,
    HEADER_PATIENT_SEARCH = 0x02,
    HEADER_PATIENT_INFO = 0x03,
    HEADER_DOCTOR_SCHEDULE = 0x04,
    HEADER_APPOINTMENT_BOOK = 0x05,
    HEADER_MEDICAL_RECORD = 0x06,
    HEADER_RESPONSE_OK = 0x80,
    HEADER_RESPONSE_ERROR = 0x81
};

#pragma pack(push, 1)

// Patient registration packet
struct TPacketPatientRegister {
    uint8_t bHeader;
    char name[64];
    char surname[64];
    char phone[16];
    char address[128];
    uint8_t age;
    char gender; // 'M' or 'F'
    char blood_type[4];
};

// Patient search packet
struct TPacketPatientSearch {
    uint8_t bHeader;
    char search_term[64];
    uint8_t search_type; // 0=name, 1=phone, 2=id
};

// Patient information response
struct TPacketPatientInfo {
    uint8_t bHeader;
    uint32_t patient_id;
    char name[64];
    char surname[64];
    char phone[16];
    char address[128];
    uint8_t age;
    char gender;
    char blood_type[4];
    uint32_t registration_date;
};

// Doctor schedule packet
struct TPacketDoctorSchedule {
    uint8_t bHeader;
    uint32_t doctor_id;
    char doctor_name[64];
    char specialization[32];
    uint8_t day_of_week; // 0=Monday, 6=Sunday
    uint8_t hour_start;
    uint8_t hour_end;
    uint8_t available;
};

// Appointment booking packet
struct TPacketAppointmentBook {
    uint8_t bHeader;
    uint32_t patient_id;
    uint32_t doctor_id;
    uint8_t day_of_week;
    uint8_t hour;
    char reason[128];
};

// Medical record packet
struct TPacketMedicalRecord {
    uint8_t bHeader;
    uint32_t patient_id;
    uint32_t doctor_id;
    char diagnosis[256];
    char treatment[256];
    char medication[128];
    uint32_t visit_date;
    uint8_t follow_up_required;
};

// Generic response packet
struct TPacketResponse {
    uint8_t bHeader;
    uint8_t result; // 0=success, 1=error
    char message[128];
};

#pragma pack(pop)

// Helper functions for packet initialization
inline void init_patient_register(TPacketPatientRegister& packet, 
                                 const char* name, const char* surname,
                                 const char* phone, const char* address,
                                 uint8_t age, char gender, const char* blood_type) {
    packet.bHeader = HEADER_PATIENT_REGISTER;
    strncpy(packet.name, name, sizeof(packet.name) - 1);
    packet.name[sizeof(packet.name) - 1] = '\0';
    strncpy(packet.surname, surname, sizeof(packet.surname) - 1);
    packet.surname[sizeof(packet.surname) - 1] = '\0';
    strncpy(packet.phone, phone, sizeof(packet.phone) - 1);
    packet.phone[sizeof(packet.phone) - 1] = '\0';
    strncpy(packet.address, address, sizeof(packet.address) - 1);
    packet.address[sizeof(packet.address) - 1] = '\0';
    packet.age = age;
    packet.gender = gender;
    strncpy(packet.blood_type, blood_type, sizeof(packet.blood_type) - 1);
    packet.blood_type[sizeof(packet.blood_type) - 1] = '\0';
}

inline void init_patient_search(TPacketPatientSearch& packet, 
                               const char* search_term, uint8_t search_type) {
    packet.bHeader = HEADER_PATIENT_SEARCH;
    strncpy(packet.search_term, search_term, sizeof(packet.search_term) - 1);
    packet.search_term[sizeof(packet.search_term) - 1] = '\0';
    packet.search_type = search_type;
}

inline void init_response(TPacketResponse& packet, uint8_t result, const char* message) {
    packet.bHeader = (result == 0) ? HEADER_RESPONSE_OK : HEADER_RESPONSE_ERROR;
    packet.result = result;
    strncpy(packet.message, message, sizeof(packet.message) - 1);
    packet.message[sizeof(packet.message) - 1] = '\0';
}

#endif // HOSPITAL_PACKET_H