#ifndef HOSPITAL_PACKET_H
#define HOSPITAL_PACKET_H

#include <cstdint>
#include <cstring>

// Packet Headers
enum PacketHeader : uint8_t {
    HEADER_CG_HANDSHAKE = 0xff,
    HEADER_GC_HANDSHAKE = 0xff,
    
    // Patient Management
    HEADER_CG_PATIENT_REGISTER = 0x01,
    HEADER_GC_PATIENT_REGISTER = 0x02,
    HEADER_CG_PATIENT_LOGIN = 0x03,
    HEADER_GC_PATIENT_LOGIN = 0x04,
    
    // Appointment System
    HEADER_CG_APPOINTMENT_REQUEST = 0x05,
    HEADER_GC_APPOINTMENT_REQUEST = 0x06,
    HEADER_CG_APPOINTMENT_LIST = 0x07,
    HEADER_GC_APPOINTMENT_LIST = 0x08,
    
    // Doctor Information
    HEADER_CG_DOCTOR_LIST = 0x09,
    HEADER_GC_DOCTOR_LIST = 0x0a,
    
    // Medical Records
    HEADER_CG_MEDICAL_RECORD = 0x0b,
    HEADER_GC_MEDICAL_RECORD = 0x0c,
    
    // General Response
    HEADER_GC_RESPONSE = 0xfd,
    HEADER_GC_ERROR = 0xfe
};

// Response Codes
enum ResponseCode : uint8_t {
    RESPONSE_SUCCESS = 0x00,
    RESPONSE_ERROR_INVALID_DATA = 0x01,
    RESPONSE_ERROR_PATIENT_NOT_FOUND = 0x02,
    RESPONSE_ERROR_DOCTOR_NOT_AVAILABLE = 0x03,
    RESPONSE_ERROR_APPOINTMENT_CONFLICT = 0x04,
    RESPONSE_ERROR_DATABASE_ERROR = 0x05
};

// Specialization Types
enum Specialization : uint8_t {
    SPECIALIZATION_GENERAL = 0x01,
    SPECIALIZATION_CARDIOLOGY = 0x02,
    SPECIALIZATION_NEUROLOGY = 0x03,
    SPECIALIZATION_ORTHOPEDICS = 0x04,
    SPECIALIZATION_PEDIATRICS = 0x05,
    SPECIALIZATION_DERMATOLOGY = 0x06
};

#pragma pack(push, 1)

// Handshake Packets (keeping original structure)
struct TPacketCGHandshake {
    uint8_t  bHeader;
    uint32_t dwHandshake;
    uint32_t dwTime;
    int32_t  lDelta;
};

struct TPacketGCHandshake {
    uint8_t  bHeader;
    uint32_t dwHandshake;
    uint32_t dwTime;
    int32_t  lDelta;
};

// Patient Registration
struct TPacketCGPatientRegister {
    uint8_t  bHeader;
    char     szName[32];
    char     szSurname[32];
    char     szTC[11];        // Turkish ID number
    char     szPhone[15];
    char     szAddress[64];
    uint8_t  bAge;
    uint8_t  bGender;         // 0: Male, 1: Female
};

struct TPacketGCPatientRegister {
    uint8_t  bHeader;
    uint8_t  bResponseCode;
    uint32_t dwPatientID;
};

// Patient Login
struct TPacketCGPatientLogin {
    uint8_t  bHeader;
    char     szTC[11];
};

struct TPacketGCPatientLogin {
    uint8_t  bHeader;
    uint8_t  bResponseCode;
    uint32_t dwPatientID;
    char     szName[32];
    char     szSurname[32];
};

// Appointment Request
struct TPacketCGAppointmentRequest {
    uint8_t  bHeader;
    uint32_t dwPatientID;
    uint32_t dwDoctorID;
    uint32_t dwDateTime;      // Unix timestamp
    char     szReason[64];
};

struct TPacketGCAppointmentRequest {
    uint8_t  bHeader;
    uint8_t  bResponseCode;
    uint32_t dwAppointmentID;
};

// Appointment List
struct TPacketCGAppointmentList {
    uint8_t  bHeader;
    uint32_t dwPatientID;
};

struct AppointmentInfo {
    uint32_t dwAppointmentID;
    uint32_t dwDoctorID;
    uint32_t dwDateTime;
    char     szDoctorName[32];
    char     szSpecialization[32];
    char     szReason[64];
    uint8_t  bStatus;         // 0: Pending, 1: Confirmed, 2: Completed, 3: Cancelled
};

struct TPacketGCAppointmentList {
    uint8_t  bHeader;
    uint8_t  bResponseCode;
    uint8_t  bAppointmentCount;
    AppointmentInfo appointments[10]; // Max 10 appointments per response
};

// Doctor List
struct TPacketCGDoctorList {
    uint8_t  bHeader;
    uint8_t  bSpecialization; // 0 for all
};

struct DoctorInfo {
    uint32_t dwDoctorID;
    char     szName[32];
    char     szSurname[32];
    uint8_t  bSpecialization;
    char     szSpecializationName[32];
    uint8_t  bAvailable;      // 0: Not available, 1: Available
};

struct TPacketGCDoctorList {
    uint8_t  bHeader;
    uint8_t  bResponseCode;
    uint8_t  bDoctorCount;
    DoctorInfo doctors[20];   // Max 20 doctors per response
};

// Medical Record
struct TPacketCGMedicalRecord {
    uint8_t  bHeader;
    uint32_t dwPatientID;
};

struct MedicalRecordInfo {
    uint32_t dwRecordID;
    uint32_t dwDateTime;
    char     szDoctorName[32];
    char     szDiagnosis[64];
    char     szTreatment[128];
    char     szNotes[128];
};

struct TPacketGCMedicalRecord {
    uint8_t  bHeader;
    uint8_t  bResponseCode;
    uint8_t  bRecordCount;
    MedicalRecordInfo records[10]; // Max 10 records per response
};

// General Response
struct TPacketGCResponse {
    uint8_t  bHeader;
    uint8_t  bResponseCode;
    char     szMessage[64];
};

#pragma pack(pop)

#endif // HOSPITAL_PACKET_H