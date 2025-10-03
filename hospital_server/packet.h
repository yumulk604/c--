#ifndef HOSPITAL_PACKET_H
#define HOSPITAL_PACKET_H

#include <cstdint>
#include <cstring>

// Paket başlıkları
enum PacketHeader : uint8_t {
    HEADER_CG_HANDSHAKE = 0xff,
    HEADER_GC_HANDSHAKE = 0xff,
    HEADER_CG_REGISTER_PATIENT = 0x01,
    HEADER_GC_REGISTER_PATIENT = 0x02,
    HEADER_CG_CREATE_APPOINTMENT = 0x03,
    HEADER_GC_CREATE_APPOINTMENT = 0x04,
    HEADER_CG_LIST_APPOINTMENTS = 0x05,
    HEADER_GC_LIST_APPOINTMENTS = 0x06,
    HEADER_CG_LIST_DOCTORS = 0x07,
    HEADER_GC_LIST_DOCTORS = 0x08,
    HEADER_CG_QUERY_PATIENT = 0x09,
    HEADER_GC_QUERY_PATIENT = 0x0A,
    HEADER_GC_ERROR = 0xFE
};

// Sonuç kodları
enum ResultCode : uint8_t {
    RESULT_SUCCESS = 0,
    RESULT_FAILED = 1,
    RESULT_PATIENT_EXISTS = 2,
    RESULT_PATIENT_NOT_FOUND = 3,
    RESULT_DOCTOR_NOT_FOUND = 4,
    RESULT_INVALID_DATE = 5
};

#pragma pack(push, 1)

// Handshake paketleri
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

// Hasta kayıt paketleri
struct TPacketCGRegisterPatient {
    uint8_t bHeader;
    char szName[32];
    char szSurname[32];
    char szTCNo[12];
    uint8_t bAge;
    char szPhone[16];
};

struct TPacketGCRegisterPatient {
    uint8_t bHeader;
    uint8_t bResult;
    uint32_t dwPatientID;
    char szMessage[128];
};

// Randevu oluşturma paketleri
struct TPacketCGCreateAppointment {
    uint8_t bHeader;
    uint32_t dwPatientID;
    uint32_t dwDoctorID;
    char szDate[11];      // YYYY-MM-DD
    char szTime[6];       // HH:MM
    char szComplaint[256];
};

struct TPacketGCCreateAppointment {
    uint8_t bHeader;
    uint8_t bResult;
    uint32_t dwAppointmentID;
    char szMessage[128];
};

// Randevu listesi paketleri
struct TPacketCGListAppointments {
    uint8_t bHeader;
    uint32_t dwPatientID;
};

struct TAppointmentInfo {
    uint32_t dwAppointmentID;
    uint32_t dwDoctorID;
    char szDoctorName[64];
    char szDate[11];
    char szTime[6];
    char szComplaint[256];
};

struct TPacketGCListAppointments {
    uint8_t bHeader;
    uint8_t bResult;
    uint32_t dwCount;
    // TAppointmentInfo appointments[dwCount] follows
};

// Doktor listesi paketleri
struct TPacketCGListDoctors {
    uint8_t bHeader;
};

struct TDoctorInfo {
    uint32_t dwDoctorID;
    char szName[64];
    char szSpecialty[64];
    char szPhone[16];
};

struct TPacketGCListDoctors {
    uint8_t bHeader;
    uint8_t bResult;
    uint32_t dwCount;
    // TDoctorInfo doctors[dwCount] follows
};

// Hasta bilgisi sorgulama paketleri
struct TPacketCGQueryPatient {
    uint8_t bHeader;
    uint32_t dwPatientID;
};

struct TPatientInfo {
    uint32_t dwPatientID;
    char szName[32];
    char szSurname[32];
    char szTCNo[12];
    uint8_t bAge;
    char szPhone[16];
    uint32_t dwRegistrationDate;
};

struct TPacketGCQueryPatient {
    uint8_t bHeader;
    uint8_t bResult;
    TPatientInfo patientInfo;
};

// Hata paketi
struct TPacketGCError {
    uint8_t bHeader;
    char szErrorMessage[256];
};

#pragma pack(pop)

#endif // HOSPITAL_PACKET_H
