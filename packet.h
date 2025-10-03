#ifndef HOSPITAL_PACKET_H
#define HOSPITAL_PACKET_H

#include <cstdint>
#include <string>

// Hospital Management System Protocol Headers
enum PacketHeader : uint8_t {
    // Client to Server packets
    HEADER_CS_LOGIN = 0x01,
    HEADER_CS_PATIENT_REGISTER = 0x02,
    HEADER_CS_PATIENT_SEARCH = 0x03,
    HEADER_CS_PATIENT_UPDATE = 0x04,
    HEADER_CS_PATIENT_DELETE = 0x05,
    HEADER_CS_DOCTOR_REGISTER = 0x06,
    HEADER_CS_DOCTOR_LIST = 0x07,
    HEADER_CS_APPOINTMENT_BOOK = 0x08,
    HEADER_CS_APPOINTMENT_LIST = 0x09,
    HEADER_CS_APPOINTMENT_CANCEL = 0x0A,
    HEADER_CS_MEDICAL_RECORD_ADD = 0x0B,
    HEADER_CS_MEDICAL_RECORD_GET = 0x0C,
    HEADER_CS_LOGOUT = 0x0D,

    // Server to Client packets
    HEADER_SC_LOGIN_RESPONSE = 0x81,
    HEADER_SC_PATIENT_DATA = 0x82,
    HEADER_SC_PATIENT_LIST = 0x83,
    HEADER_SC_DOCTOR_LIST = 0x84,
    HEADER_SC_APPOINTMENT_CONFIRM = 0x85,
    HEADER_SC_APPOINTMENT_LIST = 0x86,
    HEADER_SC_MEDICAL_RECORD = 0x87,
    HEADER_SC_ERROR = 0xFF
};

#pragma pack(push, 1)

// Authentication packets
struct TPacketCSLogin {
    uint8_t bHeader;
    char szUsername[32];
    char szPassword[32];
};

struct TPacketSCLoginResponse {
    uint8_t bHeader;
    uint8_t bSuccess;  // 0 = fail, 1 = success
    uint32_t dwUserID;
    char szUserType[16];  // "admin", "doctor", "patient"
};

// Patient management packets
struct TPacketCSPatientRegister {
    uint8_t bHeader;
    char szName[64];
    char szSurname[64];
    char szTCNumber[12];  // Turkish ID number
    char szBirthDate[11];  // YYYY-MM-DD format
    char szPhone[16];
    char szAddress[256];
    char szGender;  // M/F
};

struct TPacketCSPatientSearch {
    uint8_t bHeader;
    char szSearchTerm[64];  // Name, surname, or TC number
};

struct TPacketCSPatientUpdate {
    uint8_t bHeader;
    uint32_t dwPatientID;
    char szName[64];
    char szSurname[64];
    char szTCNumber[12];
    char szBirthDate[11];
    char szPhone[16];
    char szAddress[256];
    char szGender;
};

struct TPacketCSPatientDelete {
    uint8_t bHeader;
    uint32_t dwPatientID;
};

struct TPacketSCPatientData {
    uint8_t bHeader;
    uint32_t dwPatientID;
    char szName[64];
    char szSurname[64];
    char szTCNumber[12];
    char szBirthDate[11];
    char szPhone[16];
    char szAddress[256];
    char szGender;
    char szRegisterDate[20];  // YYYY-MM-DD HH:MM:SS
};

struct TPacketSCPatientList {
    uint8_t bHeader;
    uint32_t dwCount;
    // Followed by TPacketSCPatientData structures
};

// Doctor management packets
struct TPacketCSRegisterDoctor {
    uint8_t bHeader;
    char szName[64];
    char szSurname[64];
    char szSpecialty[64];
    char szPhone[16];
    char szEmail[128];
};

struct TPacketCSListDoctors {
    uint8_t bHeader;
};

struct TPacketSCDoctorList {
    uint8_t bHeader;
    uint32_t dwCount;
    // Followed by doctor data structures
};

// Appointment packets
struct TPacketCSBookAppointment {
    uint8_t bHeader;
    uint32_t dwPatientID;
    uint32_t dwDoctorID;
    char szAppointmentDate[20];  // YYYY-MM-DD HH:MM format
    char szNotes[256];
};

struct TPacketCSListAppointments {
    uint8_t bHeader;
    uint32_t dwPatientID;  // 0 for all appointments
};

struct TPacketCSCancelAppointment {
    uint8_t bHeader;
    uint32_t dwAppointmentID;
};

struct TPacketSCAppointmentConfirm {
    uint8_t bHeader;
    uint32_t dwAppointmentID;
    uint32_t dwPatientID;
    uint32_t dwDoctorID;
    char szAppointmentDate[20];
    char szNotes[256];
    char szStatus[16];  // "confirmed", "cancelled", "completed"
};

struct TPacketSCAppointmentList {
    uint8_t bHeader;
    uint32_t dwCount;
    // Followed by TPacketSCAppointmentConfirm structures
};

// Medical record packets
struct TPacketCSAddMedicalRecord {
    uint8_t bHeader;
    uint32_t dwPatientID;
    uint32_t dwDoctorID;
    char szDiagnosis[512];
    char szTreatment[512];
    char szMedications[512];
    char szNotes[512];
};

struct TPacketCSGetMedicalRecords {
    uint8_t bHeader;
    uint32_t dwPatientID;
};

struct TPacketSCMedicalRecord {
    uint8_t bHeader;
    uint32_t dwRecordID;
    uint32_t dwPatientID;
    uint32_t dwDoctorID;
    char szDoctorName[128];
    char szDiagnosis[512];
    char szTreatment[512];
    char szMedications[512];
    char szNotes[512];
    char szRecordDate[20];  // YYYY-MM-DD HH:MM:SS
};

// Error packet
struct TPacketSCError {
    uint8_t bHeader;
    char szErrorMessage[256];
};

// Handshake packets (for compatibility)
enum PacketHeaderCompat : uint8_t {
    HEADER_CG_HANDSHAKE = 0xff,
    HEADER_GC_HANDSHAKE = 0xff
};

struct TPacketCGHandshake {
    uint8_t bHeader;
    uint32_t dwHandshake;
    uint32_t dwTime;
    int32_t lDelta;
};

struct TPacketGCHandshake {
    uint8_t bHeader;
    uint32_t dwHandshake;
    uint32_t dwTime;
    int32_t lDelta;
};

#pragma pack(pop)

#endif // HOSPITAL_PACKET_H