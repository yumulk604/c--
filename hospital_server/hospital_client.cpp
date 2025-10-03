#include "packet.h"
#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <ctime>

int g_socket = -1;

bool ConnectToServer(const char* ip, int port) {
    g_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_socket == -1) {
        perror("socket");
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        std::cerr << "Geçersiz adres" << std::endl;
        return false;
    }

    if (connect(g_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        perror("connect");
        return false;
    }

    std::cout << "Sunucuya bağlandı: " << ip << ":" << port << std::endl;
    return true;
}

bool DoHandshake() {
    TPacketGCHandshake in_packet{};
    ssize_t received = recv(g_socket, &in_packet, sizeof(in_packet), 0);
    
    if (received != sizeof(in_packet)) {
        std::cerr << "Handshake alınamadı" << std::endl;
        return false;
    }

    if (in_packet.bHeader != HEADER_GC_HANDSHAKE) {
        std::cerr << "Geçersiz handshake paketi" << std::endl;
        return false;
    }

    TPacketCGHandshake out_packet{};
    out_packet.bHeader = HEADER_CG_HANDSHAKE;
    out_packet.dwHandshake = in_packet.dwHandshake;
    out_packet.dwTime = static_cast<uint32_t>(time(nullptr));
    out_packet.lDelta = 0;

    if (send(g_socket, &out_packet, sizeof(out_packet), 0) != sizeof(out_packet)) {
        std::cerr << "Handshake gönderilemedi" << std::endl;
        return false;
    }

    std::cout << "Handshake başarılı" << std::endl;
    return true;
}

void RegisterPatient() {
    TPacketCGRegisterPatient packet{};
    packet.bHeader = HEADER_CG_REGISTER_PATIENT;

    std::cout << "\n=== HASTA KAYIT ===" << std::endl;
    std::cout << "Ad: ";
    std::cin.getline(packet.szName, sizeof(packet.szName));
    
    std::cout << "Soyad: ";
    std::cin.getline(packet.szSurname, sizeof(packet.szSurname));
    
    std::cout << "TC No: ";
    std::cin.getline(packet.szTCNo, sizeof(packet.szTCNo));
    
    std::cout << "Yaş: ";
    int age;
    std::cin >> age;
    packet.bAge = static_cast<uint8_t>(age);
    std::cin.ignore();
    
    std::cout << "Telefon: ";
    std::cin.getline(packet.szPhone, sizeof(packet.szPhone));

    send(g_socket, &packet, sizeof(packet), 0);

    TPacketGCRegisterPatient response{};
    recv(g_socket, &response, sizeof(response), 0);

    if (response.bResult == RESULT_SUCCESS) {
        std::cout << "\n✓ " << response.szMessage << std::endl;
    } else {
        std::cout << "\n✗ Hata: " << response.szMessage << std::endl;
    }
}

void CreateAppointment() {
    TPacketCGCreateAppointment packet{};
    packet.bHeader = HEADER_CG_CREATE_APPOINTMENT;

    std::cout << "\n=== RANDEVU OLUŞTUR ===" << std::endl;
    std::cout << "Hasta ID: ";
    std::cin >> packet.dwPatientID;
    std::cin.ignore();
    
    std::cout << "Doktor ID: ";
    std::cin >> packet.dwDoctorID;
    std::cin.ignore();
    
    std::cout << "Tarih (YYYY-MM-DD): ";
    std::cin.getline(packet.szDate, sizeof(packet.szDate));
    
    std::cout << "Saat (HH:MM): ";
    std::cin.getline(packet.szTime, sizeof(packet.szTime));
    
    std::cout << "Şikayet: ";
    std::cin.getline(packet.szComplaint, sizeof(packet.szComplaint));

    send(g_socket, &packet, sizeof(packet), 0);

    TPacketGCCreateAppointment response{};
    recv(g_socket, &response, sizeof(response), 0);

    if (response.bResult == RESULT_SUCCESS) {
        std::cout << "\n✓ " << response.szMessage << std::endl;
    } else {
        std::cout << "\n✗ Hata: " << response.szMessage << std::endl;
    }
}

void ListAppointments() {
    TPacketCGListAppointments packet{};
    packet.bHeader = HEADER_CG_LIST_APPOINTMENTS;

    std::cout << "\n=== RANDEVU LİSTESİ ===" << std::endl;
    std::cout << "Hasta ID: ";
    std::cin >> packet.dwPatientID;
    std::cin.ignore();

    send(g_socket, &packet, sizeof(packet), 0);

    TPacketGCListAppointments response{};
    recv(g_socket, &response, sizeof(response), 0);

    if (response.bResult != RESULT_SUCCESS) {
        std::cout << "Hata: Randevular alınamadı" << std::endl;
        return;
    }

    if (response.dwCount == 0) {
        std::cout << "Kayıtlı randevu bulunamadı." << std::endl;
        return;
    }

    std::vector<TAppointmentInfo> appointments(response.dwCount);
    recv(g_socket, appointments.data(), 
         response.dwCount * sizeof(TAppointmentInfo), 0);

    std::cout << "\nToplam " << response.dwCount << " randevu:\n" << std::endl;
    for (const auto& apt : appointments) {
        std::cout << "ID: " << apt.dwAppointmentID << std::endl;
        std::cout << "  Doktor: " << apt.szDoctorName << std::endl;
        std::cout << "  Tarih: " << apt.szDate << " " << apt.szTime << std::endl;
        std::cout << "  Şikayet: " << apt.szComplaint << std::endl;
        std::cout << std::endl;
    }
}

void ListDoctors() {
    TPacketCGListDoctors packet{};
    packet.bHeader = HEADER_CG_LIST_DOCTORS;

    send(g_socket, &packet, sizeof(packet), 0);

    TPacketGCListDoctors response{};
    recv(g_socket, &response, sizeof(response), 0);

    if (response.bResult != RESULT_SUCCESS) {
        std::cout << "Hata: Doktor listesi alınamadı" << std::endl;
        return;
    }

    std::vector<TDoctorInfo> doctors(response.dwCount);
    recv(g_socket, doctors.data(), response.dwCount * sizeof(TDoctorInfo), 0);

    std::cout << "\n=== DOKTOR LİSTESİ ===" << std::endl;
    std::cout << "\nToplam " << response.dwCount << " doktor:\n" << std::endl;
    
    for (const auto& doc : doctors) {
        std::cout << "ID: " << doc.dwDoctorID << std::endl;
        std::cout << "  Ad: " << doc.szName << std::endl;
        std::cout << "  Uzmanlık: " << doc.szSpecialty << std::endl;
        std::cout << "  Telefon: " << doc.szPhone << std::endl;
        std::cout << std::endl;
    }
}

void QueryPatient() {
    TPacketCGQueryPatient packet{};
    packet.bHeader = HEADER_CG_QUERY_PATIENT;

    std::cout << "\n=== HASTA SORGULA ===" << std::endl;
    std::cout << "Hasta ID: ";
    std::cin >> packet.dwPatientID;
    std::cin.ignore();

    send(g_socket, &packet, sizeof(packet), 0);

    TPacketGCQueryPatient response{};
    recv(g_socket, &response, sizeof(response), 0);

    if (response.bResult != RESULT_SUCCESS) {
        std::cout << "Hata: Hasta bulunamadı" << std::endl;
        return;
    }

    const TPatientInfo& info = response.patientInfo;
    std::cout << "\nHasta Bilgileri:" << std::endl;
    std::cout << "  ID: " << info.dwPatientID << std::endl;
    std::cout << "  Ad Soyad: " << info.szName << " " << info.szSurname << std::endl;
    std::cout << "  TC No: " << info.szTCNo << std::endl;
    std::cout << "  Yaş: " << static_cast<int>(info.bAge) << std::endl;
    std::cout << "  Telefon: " << info.szPhone << std::endl;
    
    time_t regDate = static_cast<time_t>(info.dwRegistrationDate);
    std::cout << "  Kayıt Tarihi: " << ctime(&regDate);
}

void ShowMenu() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "    HASTANE YÖNETİM SİSTEMİ" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "1. Hasta Kaydet" << std::endl;
    std::cout << "2. Randevu Oluştur" << std::endl;
    std::cout << "3. Randevuları Listele" << std::endl;
    std::cout << "4. Doktorları Listele" << std::endl;
    std::cout << "5. Hasta Sorgula" << std::endl;
    std::cout << "0. Çıkış" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Seçim: ";
}

int main(int argc, char* argv[]) {
    const char* server_ip = "127.0.0.1";
    int server_port = 13000;

    if (argc >= 2) {
        server_ip = argv[1];
    }
    if (argc >= 3) {
        server_port = atoi(argv[2]);
    }

    if (!ConnectToServer(server_ip, server_port)) {
        return 1;
    }

    if (!DoHandshake()) {
        close(g_socket);
        return 1;
    }

    int choice;
    while (true) {
        ShowMenu();
        std::cin >> choice;
        std::cin.ignore();

        switch (choice) {
            case 1:
                RegisterPatient();
                break;
            case 2:
                CreateAppointment();
                break;
            case 3:
                ListAppointments();
                break;
            case 4:
                ListDoctors();
                break;
            case 5:
                QueryPatient();
                break;
            case 0:
                std::cout << "\nProgramdan çıkılıyor..." << std::endl;
                close(g_socket);
                return 0;
            default:
                std::cout << "Geçersiz seçim!" << std::endl;
        }
    }

    close(g_socket);
    return 0;
}
