#ifndef HOSPITAL_PACKET_H
#define HOSPITAL_PACKET_H

#include <cstdint>
#include <cstring>

// Packet Headers
enum PacketHeader : uint8_t {
    // Connection packets
    HEADER_CG_HANDSHAKE = 0x01,
    HEADER_GC_HANDSHAKE = 0x02,
    
    // Authentication packets
    HEADER_CG_LOGIN = 0x10,
    HEADER_GC_LOGIN_SUCCESS = 0x11,
    HEADER_GC_LOGIN_FAILURE = 0x12,
    
    // Patient management packets
    HEADER_CG_REGISTER_PATIENT = 0x20,
    HEADER_GC_REGISTER_PATIENT_SUCCESS = 0x21,
    HEADER_GC_REGISTER_PATIENT_FAILURE = 0x22,
    HEADER_CG_GET_PATIENT_INFO = 0x23,
    HEADER_GC_PATIENT_INFO = 0x24,
    
    // Appointment packets
    HEADER_CG_BOOK_APPOINTMENT = 0x30,
    HEADER_GC_APPOINTMENT_SUCCESS = 0x31,
    HEADER_GC_APPOINTMENT_FAILURE = 0x32,
    HEADER_CG_GET_APPOINTMENTS = 0x33,
    HEADER_GC_APPOINTMENT_LIST = 0x34,
    
    // Doctor management packets
    HEADER_CG_GET_DOCTORS = 0x40,
    HEADER_GC_DOCTOR_LIST = 0x41,
    
    // Medical record packets
    HEADER_CG_ADD_MEDICAL_RECORD = 0x50,
    HEADER_GC_MEDICAL_RECORD_SUCCESS = 0x51,
    HEADER_GC_MEDICAL_RECORD_FAILURE = 0x52,
    HEADER_CG_GET_MEDICAL_HISTORY = 0x53,
    HEADER_GC_MEDICAL_HISTORY = 0x54,
    
    // General response packets
    HEADER_GC_ERROR = 0xFF
};

// Error codes
enum ErrorCode : uint8_t {
    ERROR_NONE = 0,
    ERROR_INVALID_CREDENTIALS = 1,
    ERROR_PATIENT_NOT_FOUND = 2,
    ERROR_DOCTOR_NOT_FOUND = 3,
    ERROR_APPOINTMENT_CONFLICT = 4,
    ERROR_DATABASE_ERROR = 5,
    ERROR_INVALID_DATA = 6,
    ERROR_PERMISSION_DENIED = 7
};

// User types
enum UserType : uint8_t {
    USER_TYPE_PATIENT = 1,
    USER_TYPE_DOCTOR = 2,
    USER_TYPE_NURSE = 3,
    USER_TYPE_ADMIN = 4
};

#pragma pack(push, 1)

// Basic handshake packets
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

// Login packets
struct TPacketCGLogin {
    uint8_t bHeader;
    char szUsername[32];
    char szPassword[64];
    uint8_t bUserType;
};

struct TPacketGCLoginSuccess {
    uint8_t bHeader;
    uint32_t dwUserID;
    uint8_t bUserType;
    char szWelcomeMessage[128];
};

struct TPacketGCLoginFailure {
    uint8_t bHeader;
    uint8_t bErrorCode;
    char szErrorMessage[128];
};

// Patient registration
struct TPacketCGRegisterPatient {
    uint8_t bHeader;
    char szFirstName[32];
    char szLastName[32];
    char szDateOfBirth[11]; // YYYY-MM-DD
    char szGender[2];       // M/F
    char szPhone[16];
    char szEmail[64];
    char szAddress[128];
    char szEmergencyContact[32];
    char szEmergencyPhone[16];
};

struct TPacketGCRegisterPatientSuccess {
    uint8_t bHeader;
    uint32_t dwPatientID;
    char szMessage[128];
};

struct TPacketGCRegisterPatientFailure {
    uint8_t bHeader;
    uint8_t bErrorCode;
    char szErrorMessage[128];
};

// Patient info
struct TPacketCGGetPatientInfo {
    uint8_t bHeader;
    uint32_t dwPatientID;
};

struct TPatientInfo {
    uint32_t dwPatientID;
    char szFirstName[32];
    char szLastName[32];
    char szDateOfBirth[11];
    char szGender[2];
    char szPhone[16];
    char szEmail[64];
    char szAddress[128];
    char szEmergencyContact[32];
    char szEmergencyPhone[16];
    char szRegistrationDate[20]; // YYYY-MM-DD HH:MM:SS
};

struct TPacketGCPatientInfo {
    uint8_t bHeader;
    TPatientInfo patientInfo;
};

// Appointment booking
struct TPacketCGBookAppointment {
    uint8_t bHeader;
    uint32_t dwPatientID;
    uint32_t dwDoctorID;
    char szAppointmentDate[11]; // YYYY-MM-DD
    char szAppointmentTime[6];  // HH:MM
    char szReason[256];
};

struct TPacketGCAppointmentSuccess {
    uint8_t bHeader;
    uint32_t dwAppointmentID;
    char szMessage[128];
};

struct TPacketGCAppointmentFailure {
    uint8_t bHeader;
    uint8_t bErrorCode;
    char szErrorMessage[128];
};

// Appointment list
struct TPacketCGGetAppointments {
    uint8_t bHeader;
    uint32_t dwPatientID;
    char szStartDate[11]; // YYYY-MM-DD
    char szEndDate[11];   // YYYY-MM-DD
};

struct TAppointmentInfo {
    uint32_t dwAppointmentID;
    uint32_t dwPatientID;
    uint32_t dwDoctorID;
    char szPatientName[64];
    char szDoctorName[64];
    char szAppointmentDate[11];
    char szAppointmentTime[6];
    char szReason[256];
    char szStatus[16]; // scheduled, completed, cancelled
};

struct TPacketGCAppointmentList {
    uint8_t bHeader;
    uint16_t wCount;
    // Followed by wCount TAppointmentInfo structures
};

// Doctor list
struct TPacketCGGetDoctors {
    uint8_t bHeader;
    char szSpecialization[32]; // Empty for all doctors
};

struct TDoctorInfo {
    uint32_t dwDoctorID;
    char szFirstName[32];
    char szLastName[32];
    char szSpecialization[32];
    char szPhone[16];
    char szEmail[64];
    char szOfficeRoom[16];
    uint8_t bAvailable; // 1 if available, 0 if not
};

struct TPacketGCDoctorList {
    uint8_t bHeader;
    uint16_t wCount;
    // Followed by wCount TDoctorInfo structures
};

// Medical records
struct TPacketCGAddMedicalRecord {
    uint8_t bHeader;
    uint32_t dwPatientID;
    uint32_t dwDoctorID;
    char szDiagnosis[256];
    char szTreatment[512];
    char szPrescription[512];
    char szNotes[1024];
};

struct TPacketGCMedicalRecordSuccess {
    uint8_t bHeader;
    uint32_t dwRecordID;
    char szMessage[128];
};

struct TPacketGCMedicalRecordFailure {
    uint8_t bHeader;
    uint8_t bErrorCode;
    char szErrorMessage[128];
};

// Medical history
struct TPacketCGGetMedicalHistory {
    uint8_t bHeader;
    uint32_t dwPatientID;
    char szStartDate[11]; // YYYY-MM-DD
    char szEndDate[11];   // YYYY-MM-DD
};

struct TMedicalRecord {
    uint32_t dwRecordID;
    uint32_t dwPatientID;
    uint32_t dwDoctorID;
    char szDoctorName[64];
    char szRecordDate[20]; // YYYY-MM-DD HH:MM:SS
    char szDiagnosis[256];
    char szTreatment[512];
    char szPrescription[512];
    char szNotes[1024];
};

struct TPacketGCMedicalHistory {
    uint8_t bHeader;
    uint16_t wCount;
    // Followed by wCount TMedicalRecord structures
};

// Error packet
struct TPacketGCError {
    uint8_t bHeader;
    uint8_t bErrorCode;
    char szErrorMessage[256];
};

#pragma pack(pop)

// Helper functions
inline void SafeStrCopy(char* dest, const char* src, size_t destSize) {
    strncpy(dest, src, destSize - 1);
    dest[destSize - 1] = '\0';
}

#endif // HOSPITAL_PACKET_H