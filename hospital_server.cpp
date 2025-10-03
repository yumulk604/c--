#include "hospital_packet.h"
#include "database.h"
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

class HospitalServer {
private:
    int server_fd;
    Database db;
    bool running;
    
    uint32_t get_dword_time() {
        return static_cast<uint32_t>(time(nullptr));
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
        uint32_t currentUserID = 0;
        uint8_t currentUserType = 0;
        bool authenticated = false;
        
        while (running) {
            uint8_t header;
            ssize_t headerReceived = recv(client_fd, &header, sizeof(header), 0);
            if (headerReceived <= 0) {
                break; // Client disconnected
            }
            
            switch (header) {
                case HEADER_CG_LOGIN:
                    HandleLogin(client_fd, currentUserID, currentUserType, authenticated);
                    break;
                    
                case HEADER_CG_REGISTER_PATIENT:
                    HandleRegisterPatient(client_fd);
                    break;
                    
                case HEADER_CG_GET_PATIENT_INFO:
                    if (authenticated) HandleGetPatientInfo(client_fd);
                    else SendError(client_fd, ERROR_PERMISSION_DENIED, "Not authenticated");
                    break;
                    
                case HEADER_CG_BOOK_APPOINTMENT:
                    if (authenticated) HandleBookAppointment(client_fd);
                    else SendError(client_fd, ERROR_PERMISSION_DENIED, "Not authenticated");
                    break;
                    
                case HEADER_CG_GET_APPOINTMENTS:
                    if (authenticated) HandleGetAppointments(client_fd);
                    else SendError(client_fd, ERROR_PERMISSION_DENIED, "Not authenticated");
                    break;
                    
                case HEADER_CG_GET_DOCTORS:
                    HandleGetDoctors(client_fd);
                    break;
                    
                case HEADER_CG_ADD_MEDICAL_RECORD:
                    if (authenticated && currentUserType == USER_TYPE_DOCTOR) {
                        HandleAddMedicalRecord(client_fd);
                    } else {
                        SendError(client_fd, ERROR_PERMISSION_DENIED, "Doctor access required");
                    }
                    break;
                    
                case HEADER_CG_GET_MEDICAL_HISTORY:
                    if (authenticated) HandleGetMedicalHistory(client_fd);
                    else SendError(client_fd, ERROR_PERMISSION_DENIED, "Not authenticated");
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
    
    void HandleLogin(int client_fd, uint32_t& userID, uint8_t& userType, bool& authenticated) {
        TPacketCGLogin loginPacket;
        ssize_t received = recv(client_fd, reinterpret_cast<char*>(&loginPacket) + 1, 
                               sizeof(loginPacket) - 1, 0);
        
        if (received != sizeof(loginPacket) - 1) {
            SendError(client_fd, ERROR_INVALID_DATA, "Invalid login packet");
            return;
        }
        
        uint32_t authUserID;
        if (db.AuthenticateUser(loginPacket.szUsername, loginPacket.szPassword, 
                               loginPacket.bUserType, authUserID)) {
            userID = authUserID;
            userType = loginPacket.bUserType;
            authenticated = true;
            
            TPacketGCLoginSuccess response;
            response.bHeader = HEADER_GC_LOGIN_SUCCESS;
            response.dwUserID = userID;
            response.bUserType = userType;
            SafeStrCopy(response.szWelcomeMessage, "Welcome to Hospital Management System!", 
                       sizeof(response.szWelcomeMessage));
            
            send(client_fd, &response, sizeof(response), 0);
            std::cout << "User authenticated: " << loginPacket.szUsername << 
                        " (Type: " << static_cast<int>(userType) << ")" << std::endl;
        } else {
            TPacketGCLoginFailure response;
            response.bHeader = HEADER_GC_LOGIN_FAILURE;
            response.bErrorCode = ERROR_INVALID_CREDENTIALS;
            SafeStrCopy(response.szErrorMessage, "Invalid username, password, or user type", 
                       sizeof(response.szErrorMessage));
            
            send(client_fd, &response, sizeof(response), 0);
        }
    }
    
    void HandleRegisterPatient(int client_fd) {
        TPacketCGRegisterPatient regPacket;
        ssize_t received = recv(client_fd, reinterpret_cast<char*>(&regPacket) + 1, 
                               sizeof(regPacket) - 1, 0);
        
        if (received != sizeof(regPacket) - 1) {
            SendError(client_fd, ERROR_INVALID_DATA, "Invalid registration packet");
            return;
        }
        
        uint32_t patientID;
        if (db.RegisterPatient(regPacket, patientID)) {
            TPacketGCRegisterPatientSuccess response;
            response.bHeader = HEADER_GC_REGISTER_PATIENT_SUCCESS;
            response.dwPatientID = patientID;
            SafeStrCopy(response.szMessage, "Patient registered successfully!", 
                       sizeof(response.szMessage));
            
            send(client_fd, &response, sizeof(response), 0);
            std::cout << "New patient registered: " << regPacket.szFirstName << 
                        " " << regPacket.szLastName << " (ID: " << patientID << ")" << std::endl;
        } else {
            TPacketGCRegisterPatientFailure response;
            response.bHeader = HEADER_GC_REGISTER_PATIENT_FAILURE;
            response.bErrorCode = ERROR_DATABASE_ERROR;
            SafeStrCopy(response.szErrorMessage, "Failed to register patient", 
                       sizeof(response.szErrorMessage));
            
            send(client_fd, &response, sizeof(response), 0);
        }
    }
    
    void HandleGetPatientInfo(int client_fd) {
        TPacketCGGetPatientInfo infoPacket;
        ssize_t received = recv(client_fd, reinterpret_cast<char*>(&infoPacket) + 1, 
                               sizeof(infoPacket) - 1, 0);
        
        if (received != sizeof(infoPacket) - 1) {
            SendError(client_fd, ERROR_INVALID_DATA, "Invalid patient info request");
            return;
        }
        
        TPatientInfo patientInfo;
        if (db.GetPatientInfo(infoPacket.dwPatientID, patientInfo)) {
            TPacketGCPatientInfo response;
            response.bHeader = HEADER_GC_PATIENT_INFO;
            response.patientInfo = patientInfo;
            
            send(client_fd, &response, sizeof(response), 0);
        } else {
            SendError(client_fd, ERROR_PATIENT_NOT_FOUND, "Patient not found");
        }
    }
    
    void HandleBookAppointment(int client_fd) {
        TPacketCGBookAppointment appointmentPacket;
        ssize_t received = recv(client_fd, reinterpret_cast<char*>(&appointmentPacket) + 1, 
                               sizeof(appointmentPacket) - 1, 0);
        
        if (received != sizeof(appointmentPacket) - 1) {
            SendError(client_fd, ERROR_INVALID_DATA, "Invalid appointment packet");
            return;
        }
        
        uint32_t appointmentID;
        if (db.BookAppointment(appointmentPacket, appointmentID)) {
            TPacketGCAppointmentSuccess response;
            response.bHeader = HEADER_GC_APPOINTMENT_SUCCESS;
            response.dwAppointmentID = appointmentID;
            SafeStrCopy(response.szMessage, "Appointment booked successfully!", 
                       sizeof(response.szMessage));
            
            send(client_fd, &response, sizeof(response), 0);
            std::cout << "Appointment booked: ID " << appointmentID << std::endl;
        } else {
            TPacketGCAppointmentFailure response;
            response.bHeader = HEADER_GC_APPOINTMENT_FAILURE;
            response.bErrorCode = ERROR_APPOINTMENT_CONFLICT;
            SafeStrCopy(response.szErrorMessage, "Appointment time conflict or database error", 
                       sizeof(response.szErrorMessage));
            
            send(client_fd, &response, sizeof(response), 0);
        }
    }
    
    void HandleGetAppointments(int client_fd) {
        TPacketCGGetAppointments appointmentsPacket;
        ssize_t received = recv(client_fd, reinterpret_cast<char*>(&appointmentsPacket) + 1, 
                               sizeof(appointmentsPacket) - 1, 0);
        
        if (received != sizeof(appointmentsPacket) - 1) {
            SendError(client_fd, ERROR_INVALID_DATA, "Invalid appointments request");
            return;
        }
        
        std::vector<TAppointmentInfo> appointments;
        if (db.GetAppointments(appointmentsPacket.dwPatientID, appointmentsPacket.szStartDate, 
                              appointmentsPacket.szEndDate, appointments)) {
            TPacketGCAppointmentList response;
            response.bHeader = HEADER_GC_APPOINTMENT_LIST;
            response.wCount = static_cast<uint16_t>(appointments.size());
            
            send(client_fd, &response, sizeof(response), 0);
            
            // Send each appointment
            for (const auto& appointment : appointments) {
                send(client_fd, &appointment, sizeof(appointment), 0);
            }
        } else {
            SendError(client_fd, ERROR_DATABASE_ERROR, "Failed to retrieve appointments");
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
        
        std::vector<TDoctorInfo> doctors;
        if (db.GetDoctors(doctorsPacket.szSpecialization, doctors)) {
            TPacketGCDoctorList response;
            response.bHeader = HEADER_GC_DOCTOR_LIST;
            response.wCount = static_cast<uint16_t>(doctors.size());
            
            send(client_fd, &response, sizeof(response), 0);
            
            // Send each doctor
            for (const auto& doctor : doctors) {
                send(client_fd, &doctor, sizeof(doctor), 0);
            }
        } else {
            SendError(client_fd, ERROR_DATABASE_ERROR, "Failed to retrieve doctors");
        }
    }
    
    void HandleAddMedicalRecord(int client_fd) {
        TPacketCGAddMedicalRecord recordPacket;
        ssize_t received = recv(client_fd, reinterpret_cast<char*>(&recordPacket) + 1, 
                               sizeof(recordPacket) - 1, 0);
        
        if (received != sizeof(recordPacket) - 1) {
            SendError(client_fd, ERROR_INVALID_DATA, "Invalid medical record packet");
            return;
        }
        
        uint32_t recordID;
        if (db.AddMedicalRecord(recordPacket, recordID)) {
            TPacketGCMedicalRecordSuccess response;
            response.bHeader = HEADER_GC_MEDICAL_RECORD_SUCCESS;
            response.dwRecordID = recordID;
            SafeStrCopy(response.szMessage, "Medical record added successfully!", 
                       sizeof(response.szMessage));
            
            send(client_fd, &response, sizeof(response), 0);
            std::cout << "Medical record added: ID " << recordID << std::endl;
        } else {
            TPacketGCMedicalRecordFailure response;
            response.bHeader = HEADER_GC_MEDICAL_RECORD_FAILURE;
            response.bErrorCode = ERROR_DATABASE_ERROR;
            SafeStrCopy(response.szErrorMessage, "Failed to add medical record", 
                       sizeof(response.szErrorMessage));
            
            send(client_fd, &response, sizeof(response), 0);
        }
    }
    
    void HandleGetMedicalHistory(int client_fd) {
        TPacketCGGetMedicalHistory historyPacket;
        ssize_t received = recv(client_fd, reinterpret_cast<char*>(&historyPacket) + 1, 
                               sizeof(historyPacket) - 1, 0);
        
        if (received != sizeof(historyPacket) - 1) {
            SendError(client_fd, ERROR_INVALID_DATA, "Invalid medical history request");
            return;
        }
        
        std::vector<TMedicalRecord> records;
        if (db.GetMedicalHistory(historyPacket.dwPatientID, historyPacket.szStartDate, 
                                historyPacket.szEndDate, records)) {
            TPacketGCMedicalHistory response;
            response.bHeader = HEADER_GC_MEDICAL_HISTORY;
            response.wCount = static_cast<uint16_t>(records.size());
            
            send(client_fd, &response, sizeof(response), 0);
            
            // Send each record
            for (const auto& record : records) {
                send(client_fd, &record, sizeof(record), 0);
            }
        } else {
            SendError(client_fd, ERROR_DATABASE_ERROR, "Failed to retrieve medical history");
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
    HospitalServer() : server_fd(-1), running(false) {
        srand(static_cast<unsigned int>(time(nullptr)));
    }
    
    ~HospitalServer() {
        Stop();
    }
    
    bool Start(int port = 13000) {
        // Connect to database
        if (!db.Connect("localhost", "hospital_user", "hospital_pass", "hospital_management")) {
            std::cerr << "Failed to connect to database. Please ensure MySQL is running and configured." << std::endl;
            std::cerr << "Create database user with: CREATE USER 'hospital_user'@'localhost' IDENTIFIED BY 'hospital_pass';" << std::endl;
            std::cerr << "Grant privileges with: GRANT ALL PRIVILEGES ON hospital_management.* TO 'hospital_user'@'localhost';" << std::endl;
            return false;
        }
        
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
        std::cout << "Hospital Management Server started on port " << port << std::endl;
        std::cout << "Database connected successfully" << std::endl;
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
            clientThreads.emplace_back(&HospitalServer::HandleClient, this, client_fd, client_addr);
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
        db.Disconnect();
        std::cout << "Hospital server stopped" << std::endl;
    }
};

// Global server instance for signal handling
HospitalServer* g_server = nullptr;

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
    
    HospitalServer server;
    g_server = &server;
    
    if (!server.Start(port)) {
        std::cerr << "Failed to start hospital server" << std::endl;
        return 1;
    }
    
    server.Run();
    
    return 0;
}