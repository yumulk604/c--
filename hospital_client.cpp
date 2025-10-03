#include "hospital_packet.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <ctime>

class HospitalClient {
private:
    int client_fd;
    bool connected;
    uint32_t patientID;

public:
    HospitalClient() : client_fd(-1), connected(false), patientID(0) {}
    
    ~HospitalClient() {
        if (client_fd != -1) {
            close(client_fd);
        }
    }
    
    bool connectToServer(const char* server_ip, int port) {
        client_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (client_fd == -1) {
            perror("socket");
            return false;
        }
        
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
        
        if (connect(client_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
            perror("connect");
            return false;
        }
        
        // Handshake
        uint32_t handshake = static_cast<uint32_t>(rand());
        TPacketGCHandshake handshake_packet{};
        handshake_packet.bHeader = HEADER_GC_HANDSHAKE;
        handshake_packet.dwHandshake = handshake;
        handshake_packet.dwTime = static_cast<uint32_t>(time(nullptr));
        handshake_packet.lDelta = 0;
        
        if (send(client_fd, &handshake_packet, sizeof(handshake_packet), 0) != sizeof(handshake_packet)) {
            std::cerr << "Failed to send handshake" << std::endl;
            return false;
        }
        
        TPacketCGHandshake response{};
        ssize_t received = recv(client_fd, &response, sizeof(response), 0);
        if (received != sizeof(response)) {
            std::cerr << "Invalid handshake response" << std::endl;
            return false;
        }
        
        if (response.bHeader != HEADER_CG_HANDSHAKE || response.dwHandshake != handshake) {
            std::cerr << "Handshake mismatch" << std::endl;
            return false;
        }
        
        connected = true;
        std::cout << "Connected to hospital server successfully!" << std::endl;
        return true;
    }
    
    void registerPatient() {
        TPacketCGPatientRegister packet{};
        packet.bHeader = HEADER_CG_PATIENT_REGISTER;
        
        std::cout << "=== Patient Registration ===" << std::endl;
        std::cout << "Name: ";
        std::cin.getline(packet.szName, sizeof(packet.szName));
        std::cout << "Surname: ";
        std::cin.getline(packet.szSurname, sizeof(packet.szSurname));
        std::cout << "TC Number: ";
        std::cin.getline(packet.szTC, sizeof(packet.szTC));
        std::cout << "Phone: ";
        std::cin.getline(packet.szPhone, sizeof(packet.szPhone));
        std::cout << "Address: ";
        std::cin.getline(packet.szAddress, sizeof(packet.szAddress));
        std::cout << "Age: ";
        std::cin >> packet.bAge;
        std::cin.ignore();
        std::cout << "Gender (0: Male, 1: Female): ";
        std::cin >> packet.bGender;
        std::cin.ignore();
        
        send(client_fd, &packet, sizeof(packet), 0);
        
        TPacketGCPatientRegister response{};
        recv(client_fd, &response, sizeof(response), 0);
        
        if (response.bResponseCode == RESPONSE_SUCCESS) {
            patientID = response.dwPatientID;
            std::cout << "Registration successful! Patient ID: " << patientID << std::endl;
        } else {
            std::cout << "Registration failed!" << std::endl;
        }
    }
    
    void loginPatient() {
        TPacketCGPatientLogin packet{};
        packet.bHeader = HEADER_CG_PATIENT_LOGIN;
        
        std::cout << "=== Patient Login ===" << std::endl;
        std::cout << "TC Number: ";
        std::cin.getline(packet.szTC, sizeof(packet.szTC));
        
        send(client_fd, &packet, sizeof(packet), 0);
        
        TPacketGCPatientLogin response{};
        recv(client_fd, &response, sizeof(response), 0);
        
        if (response.bResponseCode == RESPONSE_SUCCESS) {
            patientID = response.dwPatientID;
            std::cout << "Login successful! Welcome " << response.szName << " " << response.szSurname << std::endl;
        } else {
            std::cout << "Login failed!" << std::endl;
        }
    }
    
    void requestAppointment() {
        if (patientID == 0) {
            std::cout << "Please login first!" << std::endl;
            return;
        }
        
        TPacketCGAppointmentRequest packet{};
        packet.bHeader = HEADER_CG_APPOINTMENT_REQUEST;
        packet.dwPatientID = patientID;
        
        std::cout << "=== Appointment Request ===" << std::endl;
        std::cout << "Doctor ID: ";
        std::cin >> packet.dwDoctorID;
        std::cin.ignore();
        
        std::cout << "Appointment Date/Time (Unix timestamp): ";
        std::cin >> packet.dwDateTime;
        std::cin.ignore();
        
        std::cout << "Reason: ";
        std::cin.getline(packet.szReason, sizeof(packet.szReason));
        
        send(client_fd, &packet, sizeof(packet), 0);
        
        TPacketGCAppointmentRequest response{};
        ssize_t received = recv(client_fd, &response, sizeof(response), 0);
        
        if (received != sizeof(response)) {
            std::cout << "Failed to receive appointment response" << std::endl;
            return;
        }
        
        if (response.bResponseCode == RESPONSE_SUCCESS) {
            std::cout << "Appointment requested successfully! Appointment ID: " << response.dwAppointmentID << std::endl;
        } else {
            std::cout << "Appointment request failed!" << std::endl;
        }
    }
    
    void getAppointmentList() {
        if (patientID == 0) {
            std::cout << "Please login first!" << std::endl;
            return;
        }
        
        TPacketCGAppointmentList packet{};
        packet.bHeader = HEADER_CG_APPOINTMENT_LIST;
        packet.dwPatientID = patientID;
        
        send(client_fd, &packet, sizeof(packet), 0);
        
        TPacketGCAppointmentList response{};
        recv(client_fd, &response, sizeof(response), 0);
        
        std::cout << "=== Your Appointments ===" << std::endl;
        for (int i = 0; i < response.bAppointmentCount; i++) {
            const auto& appt = response.appointments[i];
            std::cout << "ID: " << appt.dwAppointmentID << std::endl;
            std::cout << "Doctor: " << appt.szDoctorName << " (" << appt.szSpecialization << ")" << std::endl;
            std::cout << "Date/Time: " << appt.dwDateTime << std::endl;
            std::cout << "Reason: " << appt.szReason << std::endl;
            std::cout << "Status: " << (int)appt.bStatus << std::endl;
            std::cout << "---" << std::endl;
        }
    }
    
    void getDoctorList() {
        TPacketCGDoctorList packet{};
        packet.bHeader = HEADER_CG_DOCTOR_LIST;
        packet.bSpecialization = 0; // All specializations
        
        send(client_fd, &packet, sizeof(packet), 0);
        
        TPacketGCDoctorList response{};
        ssize_t received = recv(client_fd, &response, sizeof(response), 0);
        
        if (received != sizeof(response)) {
            std::cout << "Failed to receive doctor list" << std::endl;
            return;
        }
        
        std::cout << "=== Available Doctors ===" << std::endl;
        for (int i = 0; i < response.bDoctorCount; i++) {
            const auto& doctor = response.doctors[i];
            std::cout << "ID: " << doctor.dwDoctorID << std::endl;
            std::cout << "Name: " << doctor.szName << " " << doctor.szSurname << std::endl;
            std::cout << "Specialization: " << doctor.szSpecializationName << std::endl;
            std::cout << "Available: " << (doctor.bAvailable ? "Yes" : "No") << std::endl;
            std::cout << "---" << std::endl;
        }
    }
    
    void getMedicalRecords() {
        if (patientID == 0) {
            std::cout << "Please login first!" << std::endl;
            return;
        }
        
        TPacketCGMedicalRecord packet{};
        packet.bHeader = HEADER_CG_MEDICAL_RECORD;
        packet.dwPatientID = patientID;
        
        send(client_fd, &packet, sizeof(packet), 0);
        
        TPacketGCMedicalRecord response{};
        recv(client_fd, &response, sizeof(response), 0);
        
        std::cout << "=== Medical Records ===" << std::endl;
        for (int i = 0; i < response.bRecordCount; i++) {
            const auto& record = response.records[i];
            std::cout << "Record ID: " << record.dwRecordID << std::endl;
            std::cout << "Date: " << record.dwDateTime << std::endl;
            std::cout << "Doctor: " << record.szDoctorName << std::endl;
            std::cout << "Diagnosis: " << record.szDiagnosis << std::endl;
            std::cout << "Treatment: " << record.szTreatment << std::endl;
            std::cout << "Notes: " << record.szNotes << std::endl;
            std::cout << "---" << std::endl;
        }
    }
    
    void showMenu() {
        std::cout << "\n=== Hospital Management System ===" << std::endl;
        std::cout << "1. Register Patient" << std::endl;
        std::cout << "2. Login Patient" << std::endl;
        std::cout << "3. Request Appointment" << std::endl;
        std::cout << "4. View Appointments" << std::endl;
        std::cout << "5. View Doctors" << std::endl;
        std::cout << "6. View Medical Records" << std::endl;
        std::cout << "0. Exit" << std::endl;
        std::cout << "Choice: ";
    }
    
    void run() {
        if (!connectToServer("127.0.0.1", 13000)) {
            std::cerr << "Failed to connect to server" << std::endl;
            return;
        }
        
        int choice;
        do {
            showMenu();
            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Invalid input!" << std::endl;
                continue;
            }
            std::cin.ignore();
            
            switch (choice) {
                case 1:
                    registerPatient();
                    break;
                case 2:
                    loginPatient();
                    break;
                case 3:
                    requestAppointment();
                    break;
                case 4:
                    getAppointmentList();
                    break;
                case 5:
                    getDoctorList();
                    break;
                case 6:
                    getMedicalRecords();
                    break;
                case 0:
                    std::cout << "Goodbye!" << std::endl;
                    break;
                default:
                    std::cout << "Invalid choice!" << std::endl;
                    break;
            }
        } while (choice != 0);
    }
};

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));
    
    HospitalClient client;
    client.run();
    
    return 0;
}