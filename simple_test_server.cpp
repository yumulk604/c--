#include "hospital_packet.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <thread>
#include <vector>
#include <signal.h>
#include <map>

// Simple in-memory storage for testing
struct SimplePatient {
    uint32_t id;
    std::string firstName;
    std::string lastName;
    std::string email;
    std::string phone;
};

struct SimpleDoctor {
    uint32_t id;
    std::string firstName;
    std::string lastName;
    std::string specialization;
    std::string office;
};

class SimpleHospitalServer {
private:
    int server_fd;
    bool running;
    std::map<uint32_t, SimplePatient> patients;
    std::map<uint32_t, SimpleDoctor> doctors;
    uint32_t nextPatientId;
    uint32_t nextDoctorId;
    
    uint32_t get_dword_time() {
        return static_cast<uint32_t>(time(nullptr));
    }
    
    void InitializeSampleData() {
        // Add sample doctors
        doctors[1] = {1, "John", "Smith", "Cardiology", "C201"};
        doctors[2] = {2, "Sarah", "Johnson", "Internal Medicine", "IM101"};
        doctors[3] = {3, "Michael", "Williams", "Emergency Medicine", "ER01"};
        nextDoctorId = 4;
        
        // Add sample patient
        patients[1] = {1, "Jane", "Doe", "jane.doe@email.com", "555-0301"};
        nextPatientId = 2;
    }
    
    void HandleClient(int client_fd, sockaddr_in client_addr) {
        std::cout << "Client connected: " << inet_ntoa(client_addr.sin_addr) << std::endl;
        
        // Send handshake
        uint32_t handshake = static_cast<uint32_t>(rand());
        TPacketGCHandshake out_packet{};
        out_packet.bHeader = HEADER_GC_HANDSHAKE;
        out_packet.dwHandshake = handshake;
        out_packet.dwTime = get_dword_time();
        out_packet.lDelta = 0;
        
        if (send(client_fd, &out_packet, sizeof(out_packet), 0) != sizeof(out_packet)) {
            std::cerr << "Failed to send handshake" << std::endl;
            close(client_fd);
            return;
        }
        
        // Receive handshake response
        TPacketCGHandshake in_packet{};
        ssize_t received = recv(client_fd, &in_packet, sizeof(in_packet), 0);
        if (received != sizeof(in_packet)) {
            std::cerr << "Invalid handshake size" << std::endl;
            close(client_fd);
            return;
        }
        
        if (in_packet.bHeader != HEADER_CG_HANDSHAKE || in_packet.dwHandshake != handshake) {
            std::cerr << "Handshake mismatch" << std::endl;
            close(client_fd);
            return;
        }
        
        std::cout << "Handshake successful with client" << std::endl;
        
        // Main communication loop
        bool authenticated = false;
        
        while (running) {
            uint8_t header;
            ssize_t headerReceived = recv(client_fd, &header, sizeof(header), 0);
            if (headerReceived <= 0) {
                break; // Client disconnected
            }
            
            switch (header) {
                case HEADER_CG_LOGIN:
                    HandleLogin(client_fd, authenticated);
                    break;
                    
                case HEADER_CG_REGISTER_PATIENT:
                    HandleRegisterPatient(client_fd);
                    break;
                    
                case HEADER_CG_GET_PATIENT_INFO:
                    HandleGetPatientInfo(client_fd);
                    break;
                    
                case HEADER_CG_GET_DOCTORS:
                    HandleGetDoctors(client_fd);
                    break;
                    
                default:
                    std::cerr << "Unknown packet header: " << static_cast<int>(header) << std::endl;
                    SendError(client_fd, ERROR_INVALID_DATA, "Unknown command");
                    break;
            }
        }
        
        std::cout << "Client disconnected: " << inet_ntoa(client_addr.sin_addr) << std::endl;
        close(client_fd);
    }
    
    void HandleLogin(int client_fd, bool& authenticated) {
        TPacketCGLogin loginPacket;
        ssize_t received = recv(client_fd, reinterpret_cast<char*>(&loginPacket) + 1, 
                               sizeof(loginPacket) - 1, 0);
        
        if (received != sizeof(loginPacket) - 1) {
            SendError(client_fd, ERROR_INVALID_DATA, "Invalid login packet");
            return;
        }
        
        // Simple authentication - accept any credentials for testing
        authenticated = true;
        
        TPacketGCLoginSuccess response;
        response.bHeader = HEADER_GC_LOGIN_SUCCESS;
        response.dwUserID = 1;
        response.bUserType = loginPacket.bUserType;
        SafeStrCopy(response.szWelcomeMessage, "Welcome to Test Hospital System!", 
                   sizeof(response.szWelcomeMessage));
        
        send(client_fd, &response, sizeof(response), 0);
        std::cout << "User authenticated: " << loginPacket.szUsername << std::endl;
    }
    
    void HandleRegisterPatient(int client_fd) {
        TPacketCGRegisterPatient regPacket;
        ssize_t received = recv(client_fd, reinterpret_cast<char*>(&regPacket) + 1, 
                               sizeof(regPacket) - 1, 0);
        
        if (received != sizeof(regPacket) - 1) {
            SendError(client_fd, ERROR_INVALID_DATA, "Invalid registration packet");
            return;
        }
        
        // Add patient to memory
        SimplePatient patient;
        patient.id = nextPatientId++;
        patient.firstName = regPacket.szFirstName;
        patient.lastName = regPacket.szLastName;
        patient.email = regPacket.szEmail;
        patient.phone = regPacket.szPhone;
        
        patients[patient.id] = patient;
        
        TPacketGCRegisterPatientSuccess response;
        response.bHeader = HEADER_GC_REGISTER_PATIENT_SUCCESS;
        response.dwPatientID = patient.id;
        SafeStrCopy(response.szMessage, "Patient registered successfully in test system!", 
                   sizeof(response.szMessage));
        
        send(client_fd, &response, sizeof(response), 0);
        std::cout << "New patient registered: " << patient.firstName << 
                    " " << patient.lastName << " (ID: " << patient.id << ")" << std::endl;
    }
    
    void HandleGetPatientInfo(int client_fd) {
        TPacketCGGetPatientInfo infoPacket;
        ssize_t received = recv(client_fd, reinterpret_cast<char*>(&infoPacket) + 1, 
                               sizeof(infoPacket) - 1, 0);
        
        if (received != sizeof(infoPacket) - 1) {
            SendError(client_fd, ERROR_INVALID_DATA, "Invalid patient info request");
            return;
        }
        
        auto it = patients.find(infoPacket.dwPatientID);
        if (it != patients.end()) {
            TPacketGCPatientInfo response;
            response.bHeader = HEADER_GC_PATIENT_INFO;
            
            response.patientInfo.dwPatientID = it->second.id;
            SafeStrCopy(response.patientInfo.szFirstName, it->second.firstName.c_str(), 
                       sizeof(response.patientInfo.szFirstName));
            SafeStrCopy(response.patientInfo.szLastName, it->second.lastName.c_str(), 
                       sizeof(response.patientInfo.szLastName));
            SafeStrCopy(response.patientInfo.szEmail, it->second.email.c_str(), 
                       sizeof(response.patientInfo.szEmail));
            SafeStrCopy(response.patientInfo.szPhone, it->second.phone.c_str(), 
                       sizeof(response.patientInfo.szPhone));
            SafeStrCopy(response.patientInfo.szDateOfBirth, "1985-03-15", 
                       sizeof(response.patientInfo.szDateOfBirth));
            SafeStrCopy(response.patientInfo.szGender, "F", 
                       sizeof(response.patientInfo.szGender));
            SafeStrCopy(response.patientInfo.szRegistrationDate, "2024-10-03 10:00:00", 
                       sizeof(response.patientInfo.szRegistrationDate));
            
            send(client_fd, &response, sizeof(response), 0);
        } else {
            SendError(client_fd, ERROR_PATIENT_NOT_FOUND, "Patient not found");
        }
    }
    
    void HandleGetDoctors(int client_fd) {
        TPacketCGGetDoctors doctorsPacket;
        ssize_t received = recv(client_fd, reinterpret_cast<char*>(&doctorsPacket) + 1, 
                               sizeof(doctorsPacket) - 1, 0);
        
        if (received != sizeof(doctorsPacket) - 1) {
            SendError(client_fd, ERROR_INVALID_DATA, "Invalid doctors request");
            return;
        }
        
        TPacketGCDoctorList response;
        response.bHeader = HEADER_GC_DOCTOR_LIST;
        response.wCount = static_cast<uint16_t>(doctors.size());
        
        send(client_fd, &response, sizeof(response), 0);
        
        // Send each doctor
        for (const auto& pair : doctors) {
            TDoctorInfo doctor;
            doctor.dwDoctorID = pair.second.id;
            SafeStrCopy(doctor.szFirstName, pair.second.firstName.c_str(), sizeof(doctor.szFirstName));
            SafeStrCopy(doctor.szLastName, pair.second.lastName.c_str(), sizeof(doctor.szLastName));
            SafeStrCopy(doctor.szSpecialization, pair.second.specialization.c_str(), sizeof(doctor.szSpecialization));
            SafeStrCopy(doctor.szOfficeRoom, pair.second.office.c_str(), sizeof(doctor.szOfficeRoom));
            SafeStrCopy(doctor.szPhone, "555-0100", sizeof(doctor.szPhone));
            SafeStrCopy(doctor.szEmail, "doctor@hospital.com", sizeof(doctor.szEmail));
            doctor.bAvailable = 1;
            
            send(client_fd, &doctor, sizeof(doctor), 0);
        }
    }
    
    void SendError(int client_fd, uint8_t errorCode, const std::string& message) {
        TPacketGCError errorPacket;
        errorPacket.bHeader = HEADER_GC_ERROR;
        errorPacket.bErrorCode = errorCode;
        SafeStrCopy(errorPacket.szErrorMessage, message.c_str(), sizeof(errorPacket.szErrorMessage));
        
        send(client_fd, &errorPacket, sizeof(errorPacket), 0);
    }
    
public:
    SimpleHospitalServer() : server_fd(-1), running(false), nextPatientId(1), nextDoctorId(1) {
        srand(static_cast<unsigned int>(time(nullptr)));
        InitializeSampleData();
    }
    
    ~SimpleHospitalServer() {
        Stop();
    }
    
    bool Start(int port = 13000) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) {
            perror("socket");
            return false;
        }
        
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
            perror("bind");
            return false;
        }
        
        if (listen(server_fd, 10) == -1) {
            perror("listen");
            return false;
        }
        
        running = true;
        std::cout << "Simple Hospital Test Server started on port " << port << std::endl;
        std::cout << "This is a test version without database - data stored in memory only" << std::endl;
        std::cout << "Waiting for client connections..." << std::endl;
        
        return true;
    }
    
    void Run() {
        std::vector<std::thread> clientThreads;
        
        while (running) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
            
            if (client_fd == -1) {
                if (running) {
                    perror("accept");
                }
                continue;
            }
            
            // Handle client in separate thread
            clientThreads.emplace_back(&SimpleHospitalServer::HandleClient, this, client_fd, client_addr);
        }
        
        // Wait for all client threads to finish
        for (auto& thread : clientThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
    
    void Stop() {
        running = false;
        if (server_fd != -1) {
            close(server_fd);
            server_fd = -1;
        }
        std::cout << "Simple hospital server stopped" << std::endl;
    }
};

// Global server instance for signal handling
SimpleHospitalServer* g_server = nullptr;

void SignalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down server..." << std::endl;
    if (g_server) {
        g_server->Stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    int port = 13000;
    
    if (argc > 1) {
        port = std::atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Invalid port number. Using default port 13000." << std::endl;
            port = 13000;
        }
    }
    
    // Set up signal handlers
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    
    SimpleHospitalServer server;
    g_server = &server;
    
    if (!server.Start(port)) {
        std::cerr << "Failed to start simple hospital server" << std::endl;
        return 1;
    }
    
    server.Run();
    
    return 0;
}