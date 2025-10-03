#include "packet.h"
#include <iostream>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <ctime>
#include <cstring>

// Veri yapıları
struct Patient {
    uint32_t id;
    char name[32];
    char surname[32];
    char tcno[12];
    uint8_t age;
    char phone[16];
    uint32_t registrationDate;
};

struct Doctor {
    uint32_t id;
    char name[64];
    char specialty[64];
    char phone[16];
};

struct Appointment {
    uint32_t id;
    uint32_t patientID;
    uint32_t doctorID;
    char date[11];
    char time[6];
    char complaint[256];
};

// Global veri depoları (basit bellek tabanlı)
std::map<uint32_t, Patient> g_patients;
std::map<uint32_t, Doctor> g_doctors;
std::map<uint32_t, Appointment> g_appointments;
std::map<std::string, uint32_t> g_tcnoToPatientID;

uint32_t g_nextPatientID = 1;
uint32_t g_nextAppointmentID = 1;

uint32_t get_dword_time() {
    return static_cast<uint32_t>(time(nullptr));
}

void InitializeDoctors() {
    // Örnek doktorlar
    Doctor doc1 = {1, "Dr. Mehmet Yılmaz", "Kardiyoloji", "0555-111-2233"};
    Doctor doc2 = {2, "Dr. Ayşe Kaya", "Nöroloji", "0555-222-3344"};
    Doctor doc3 = {3, "Dr. Ahmet Demir", "Ortopedi", "0555-333-4455"};
    Doctor doc4 = {4, "Dr. Fatma Şahin", "Göz Hastalıkları", "0555-444-5566"};
    Doctor doc5 = {5, "Dr. Can Öztürk", "Dahiliye", "0555-555-6677"};
    
    g_doctors[1] = doc1;
    g_doctors[2] = doc2;
    g_doctors[3] = doc3;
    g_doctors[4] = doc4;
    g_doctors[5] = doc5;
}

bool HandleHandshake(int client_fd, uint32_t handshake) {
    TPacketCGHandshake in_packet{};
    ssize_t received = recv(client_fd, &in_packet, sizeof(in_packet), 0);
    if (received != sizeof(in_packet)) {
        std::cerr << "Geçersiz handshake boyutu" << std::endl;
        return false;
    }

    if (in_packet.bHeader != HEADER_CG_HANDSHAKE || in_packet.dwHandshake != handshake) {
        std::cerr << "Handshake uyuşmazlığı" << std::endl;
        return false;
    }

    std::cout << "Handshake başarılı" << std::endl;
    return true;
}

void HandleRegisterPatient(int client_fd) {
    TPacketCGRegisterPatient in_packet{};
    ssize_t received = recv(client_fd, &in_packet, sizeof(in_packet), 0);
    
    TPacketGCRegisterPatient out_packet{};
    out_packet.bHeader = HEADER_GC_REGISTER_PATIENT;
    
    if (received != sizeof(in_packet)) {
        out_packet.bResult = RESULT_FAILED;
        strncpy(out_packet.szMessage, "Geçersiz paket boyutu", sizeof(out_packet.szMessage) - 1);
        send(client_fd, &out_packet, sizeof(out_packet), 0);
        return;
    }

    // TC No kontrolü
    if (g_tcnoToPatientID.find(in_packet.szTCNo) != g_tcnoToPatientID.end()) {
        out_packet.bResult = RESULT_PATIENT_EXISTS;
        strncpy(out_packet.szMessage, "Bu TC No ile kayıtlı hasta mevcut", sizeof(out_packet.szMessage) - 1);
        send(client_fd, &out_packet, sizeof(out_packet), 0);
        return;
    }

    // Yeni hasta oluştur
    Patient patient{};
    patient.id = g_nextPatientID++;
    strncpy(patient.name, in_packet.szName, sizeof(patient.name) - 1);
    strncpy(patient.surname, in_packet.szSurname, sizeof(patient.surname) - 1);
    strncpy(patient.tcno, in_packet.szTCNo, sizeof(patient.tcno) - 1);
    patient.age = in_packet.bAge;
    strncpy(patient.phone, in_packet.szPhone, sizeof(patient.phone) - 1);
    patient.registrationDate = get_dword_time();

    g_patients[patient.id] = patient;
    g_tcnoToPatientID[patient.tcno] = patient.id;

    out_packet.bResult = RESULT_SUCCESS;
    out_packet.dwPatientID = patient.id;
    snprintf(out_packet.szMessage, sizeof(out_packet.szMessage), 
             "Hasta başarıyla kaydedildi. ID: %u", patient.id);
    
    send(client_fd, &out_packet, sizeof(out_packet), 0);
    std::cout << "Yeni hasta kaydedildi: " << patient.name << " " << patient.surname 
              << " (ID: " << patient.id << ")" << std::endl;
}

void HandleCreateAppointment(int client_fd) {
    TPacketCGCreateAppointment in_packet{};
    ssize_t received = recv(client_fd, &in_packet, sizeof(in_packet), 0);
    
    TPacketGCCreateAppointment out_packet{};
    out_packet.bHeader = HEADER_GC_CREATE_APPOINTMENT;
    
    if (received != sizeof(in_packet)) {
        out_packet.bResult = RESULT_FAILED;
        strncpy(out_packet.szMessage, "Geçersiz paket boyutu", sizeof(out_packet.szMessage) - 1);
        send(client_fd, &out_packet, sizeof(out_packet), 0);
        return;
    }

    // Hasta kontrolü
    if (g_patients.find(in_packet.dwPatientID) == g_patients.end()) {
        out_packet.bResult = RESULT_PATIENT_NOT_FOUND;
        strncpy(out_packet.szMessage, "Hasta bulunamadı", sizeof(out_packet.szMessage) - 1);
        send(client_fd, &out_packet, sizeof(out_packet), 0);
        return;
    }

    // Doktor kontrolü
    if (g_doctors.find(in_packet.dwDoctorID) == g_doctors.end()) {
        out_packet.bResult = RESULT_DOCTOR_NOT_FOUND;
        strncpy(out_packet.szMessage, "Doktor bulunamadı", sizeof(out_packet.szMessage) - 1);
        send(client_fd, &out_packet, sizeof(out_packet), 0);
        return;
    }

    // Randevu oluştur
    Appointment appointment{};
    appointment.id = g_nextAppointmentID++;
    appointment.patientID = in_packet.dwPatientID;
    appointment.doctorID = in_packet.dwDoctorID;
    strncpy(appointment.date, in_packet.szDate, sizeof(appointment.date) - 1);
    strncpy(appointment.time, in_packet.szTime, sizeof(appointment.time) - 1);
    strncpy(appointment.complaint, in_packet.szComplaint, sizeof(appointment.complaint) - 1);

    g_appointments[appointment.id] = appointment;

    out_packet.bResult = RESULT_SUCCESS;
    out_packet.dwAppointmentID = appointment.id;
    snprintf(out_packet.szMessage, sizeof(out_packet.szMessage), 
             "Randevu başarıyla oluşturuldu. ID: %u", appointment.id);
    
    send(client_fd, &out_packet, sizeof(out_packet), 0);
    std::cout << "Yeni randevu oluşturuldu: ID=" << appointment.id 
              << " Tarih=" << appointment.date << " " << appointment.time << std::endl;
}

void HandleListAppointments(int client_fd) {
    TPacketCGListAppointments in_packet{};
    ssize_t received = recv(client_fd, &in_packet, sizeof(in_packet), 0);
    
    if (received != sizeof(in_packet)) {
        return;
    }

    // Hastanın randevularını bul
    std::vector<TAppointmentInfo> appointments;
    for (const auto& pair : g_appointments) {
        if (pair.second.patientID == in_packet.dwPatientID) {
            TAppointmentInfo info{};
            info.dwAppointmentID = pair.second.id;
            info.dwDoctorID = pair.second.doctorID;
            
            if (g_doctors.find(pair.second.doctorID) != g_doctors.end()) {
                strncpy(info.szDoctorName, g_doctors[pair.second.doctorID].name, 
                        sizeof(info.szDoctorName) - 1);
            }
            
            strncpy(info.szDate, pair.second.date, sizeof(info.szDate) - 1);
            strncpy(info.szTime, pair.second.time, sizeof(info.szTime) - 1);
            strncpy(info.szComplaint, pair.second.complaint, sizeof(info.szComplaint) - 1);
            
            appointments.push_back(info);
        }
    }

    TPacketGCListAppointments out_packet{};
    out_packet.bHeader = HEADER_GC_LIST_APPOINTMENTS;
    out_packet.bResult = RESULT_SUCCESS;
    out_packet.dwCount = appointments.size();
    
    send(client_fd, &out_packet, sizeof(out_packet), 0);
    
    if (!appointments.empty()) {
        send(client_fd, appointments.data(), 
             appointments.size() * sizeof(TAppointmentInfo), 0);
    }
    
    std::cout << "Randevu listesi gönderildi: " << appointments.size() 
              << " randevu" << std::endl;
}

void HandleListDoctors(int client_fd) {
    TPacketCGListDoctors in_packet{};
    recv(client_fd, &in_packet, sizeof(in_packet), 0);

    std::vector<TDoctorInfo> doctors;
    for (const auto& pair : g_doctors) {
        TDoctorInfo info{};
        info.dwDoctorID = pair.second.id;
        strncpy(info.szName, pair.second.name, sizeof(info.szName) - 1);
        strncpy(info.szSpecialty, pair.second.specialty, sizeof(info.szSpecialty) - 1);
        strncpy(info.szPhone, pair.second.phone, sizeof(info.szPhone) - 1);
        doctors.push_back(info);
    }

    TPacketGCListDoctors out_packet{};
    out_packet.bHeader = HEADER_GC_LIST_DOCTORS;
    out_packet.bResult = RESULT_SUCCESS;
    out_packet.dwCount = doctors.size();
    
    send(client_fd, &out_packet, sizeof(out_packet), 0);
    
    if (!doctors.empty()) {
        send(client_fd, doctors.data(), doctors.size() * sizeof(TDoctorInfo), 0);
    }
    
    std::cout << "Doktor listesi gönderildi: " << doctors.size() 
              << " doktor" << std::endl;
}

void HandleQueryPatient(int client_fd) {
    TPacketCGQueryPatient in_packet{};
    ssize_t received = recv(client_fd, &in_packet, sizeof(in_packet), 0);
    
    TPacketGCQueryPatient out_packet{};
    out_packet.bHeader = HEADER_GC_QUERY_PATIENT;
    
    if (received != sizeof(in_packet)) {
        out_packet.bResult = RESULT_FAILED;
        send(client_fd, &out_packet, sizeof(out_packet), 0);
        return;
    }

    if (g_patients.find(in_packet.dwPatientID) == g_patients.end()) {
        out_packet.bResult = RESULT_PATIENT_NOT_FOUND;
        send(client_fd, &out_packet, sizeof(out_packet), 0);
        return;
    }

    const Patient& patient = g_patients[in_packet.dwPatientID];
    
    out_packet.bResult = RESULT_SUCCESS;
    out_packet.patientInfo.dwPatientID = patient.id;
    strncpy(out_packet.patientInfo.szName, patient.name, 
            sizeof(out_packet.patientInfo.szName) - 1);
    strncpy(out_packet.patientInfo.szSurname, patient.surname, 
            sizeof(out_packet.patientInfo.szSurname) - 1);
    strncpy(out_packet.patientInfo.szTCNo, patient.tcno, 
            sizeof(out_packet.patientInfo.szTCNo) - 1);
    out_packet.patientInfo.bAge = patient.age;
    strncpy(out_packet.patientInfo.szPhone, patient.phone, 
            sizeof(out_packet.patientInfo.szPhone) - 1);
    out_packet.patientInfo.dwRegistrationDate = patient.registrationDate;
    
    send(client_fd, &out_packet, sizeof(out_packet), 0);
    std::cout << "Hasta bilgisi gönderildi: " << patient.name << " " 
              << patient.surname << std::endl;
}

void HandleClient(int client_fd, sockaddr_in& client_addr) {
    std::cout << "İstemci bağlandı: " << inet_ntoa(client_addr.sin_addr) << std::endl;

    // Handshake gönder
    uint32_t handshake = static_cast<uint32_t>(rand());
    TPacketGCHandshake out_packet{};
    out_packet.bHeader = HEADER_GC_HANDSHAKE;
    out_packet.dwHandshake = handshake;
    out_packet.dwTime = get_dword_time();
    out_packet.lDelta = 0;

    if (send(client_fd, &out_packet, sizeof(out_packet), 0) != sizeof(out_packet)) {
        std::cerr << "Handshake gönderilemedi" << std::endl;
        close(client_fd);
        return;
    }

    if (!HandleHandshake(client_fd, handshake)) {
        close(client_fd);
        return;
    }

    // İstemci komutlarını işle
    while (true) {
        uint8_t header;
        ssize_t received = recv(client_fd, &header, sizeof(header), MSG_PEEK);
        
        if (received <= 0) {
            std::cout << "İstemci bağlantısı kesildi" << std::endl;
            break;
        }

        switch (header) {
            case HEADER_CG_REGISTER_PATIENT:
                HandleRegisterPatient(client_fd);
                break;
            case HEADER_CG_CREATE_APPOINTMENT:
                HandleCreateAppointment(client_fd);
                break;
            case HEADER_CG_LIST_APPOINTMENTS:
                HandleListAppointments(client_fd);
                break;
            case HEADER_CG_LIST_DOCTORS:
                HandleListDoctors(client_fd);
                break;
            case HEADER_CG_QUERY_PATIENT:
                HandleQueryPatient(client_fd);
                break;
            default:
                std::cerr << "Bilinmeyen paket: 0x" << std::hex 
                          << static_cast<int>(header) << std::dec << std::endl;
                // Geçersiz paketi temizle
                recv(client_fd, &header, sizeof(header), 0);
                break;
        }
    }

    close(client_fd);
}

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));
    
    InitializeDoctors();
    std::cout << "Hastane sistemi başlatılıyor..." << std::endl;
    std::cout << g_doctors.size() << " doktor yüklendi." << std::endl;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(13000);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        return 1;
    }

    std::cout << "Hastane sunucusu 13000 portunda başlatıldı" << std::endl;
    std::cout << "İstemci bağlantıları bekleniyor..." << std::endl;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), 
                               &client_len);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        HandleClient(client_fd, client_addr);
    }

    close(server_fd);
    return 0;
}
