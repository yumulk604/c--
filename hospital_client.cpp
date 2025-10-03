#include "hospital_packet.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>

class HospitalClient {
private:
    int client_fd;
    bool connected;
    bool authenticated;
    uint32_t userID;
    uint8_t userType;
    
    bool SendPacket(const void* packet, size_t size) {
        return send(client_fd, packet, size, 0) == static_cast<ssize_t>(size);
    }
    
    bool ReceivePacket(void* packet, size_t size) {
        return recv(client_fd, packet, size, 0) == static_cast<ssize_t>(size);
    }
    
    void DisplayMenu() {
        std::cout << "\n=== Hospital Management System ===" << std::endl;
        std::cout << "1. Login" << std::endl;
        std::cout << "2. Register New Patient" << std::endl;
        std::cout << "3. Get Patient Information" << std::endl;
        std::cout << "4. Book Appointment" << std::endl;
        std::cout << "5. View Appointments" << std::endl;
        std::cout << "6. View Available Doctors" << std::endl;
        std::cout << "7. Add Medical Record (Doctors only)" << std::endl;
        std::cout << "8. View Medical History" << std::endl;
        std::cout << "9. Disconnect" << std::endl;
        std::cout << "Enter your choice: ";
    }
    
    void HandleLogin() {
        TPacketCGLogin loginPacket;
        loginPacket.bHeader = HEADER_CG_LOGIN;
        
        std::cout << "Enter username: ";
        std::cin.getline(loginPacket.szUsername, sizeof(loginPacket.szUsername));
        
        std::cout << "Enter password: ";
        std::cin.getline(loginPacket.szPassword, sizeof(loginPacket.szPassword));
        
        std::cout << "User type (1=Patient, 2=Doctor, 3=Nurse, 4=Admin): ";
        int type;
        std::cin >> type;
        std::cin.ignore(); // Clear newline
        loginPacket.bUserType = static_cast<uint8_t>(type);
        
        if (!SendPacket(&loginPacket, sizeof(loginPacket))) {
            std::cerr << "Failed to send login packet" << std::endl;
            return;
        }
        
        uint8_t responseHeader;
        if (!ReceivePacket(&responseHeader, sizeof(responseHeader))) {
            std::cerr << "Failed to receive login response" << std::endl;
            return;
        }
        
        if (responseHeader == HEADER_GC_LOGIN_SUCCESS) {
            TPacketGCLoginSuccess response;
            if (ReceivePacket(reinterpret_cast<char*>(&response) + 1, sizeof(response) - 1)) {
                authenticated = true;
                userID = response.dwUserID;
                userType = response.bUserType;
                std::cout << "Login successful! " << response.szWelcomeMessage << std::endl;
                std::cout << "User ID: " << userID << ", Type: " << static_cast<int>(userType) << std::endl;
            }
        } else if (responseHeader == HEADER_GC_LOGIN_FAILURE) {
            TPacketGCLoginFailure response;
            if (ReceivePacket(reinterpret_cast<char*>(&response) + 1, sizeof(response) - 1)) {
                std::cout << "Login failed: " << response.szErrorMessage << std::endl;
            }
        }
    }
    
    void HandleRegisterPatient() {
        TPacketCGRegisterPatient regPacket;
        regPacket.bHeader = HEADER_CG_REGISTER_PATIENT;
        
        std::cout << "Enter patient information:" << std::endl;
        std::cout << "First name: ";
        std::cin.getline(regPacket.szFirstName, sizeof(regPacket.szFirstName));
        
        std::cout << "Last name: ";
        std::cin.getline(regPacket.szLastName, sizeof(regPacket.szLastName));
        
        std::cout << "Date of birth (YYYY-MM-DD): ";
        std::cin.getline(regPacket.szDateOfBirth, sizeof(regPacket.szDateOfBirth));
        
        std::cout << "Gender (M/F): ";
        std::cin.getline(regPacket.szGender, sizeof(regPacket.szGender));
        
        std::cout << "Phone: ";
        std::cin.getline(regPacket.szPhone, sizeof(regPacket.szPhone));
        
        std::cout << "Email: ";
        std::cin.getline(regPacket.szEmail, sizeof(regPacket.szEmail));
        
        std::cout << "Address: ";
        std::cin.getline(regPacket.szAddress, sizeof(regPacket.szAddress));
        
        std::cout << "Emergency contact name: ";
        std::cin.getline(regPacket.szEmergencyContact, sizeof(regPacket.szEmergencyContact));
        
        std::cout << "Emergency contact phone: ";
        std::cin.getline(regPacket.szEmergencyPhone, sizeof(regPacket.szEmergencyPhone));
        
        if (!SendPacket(&regPacket, sizeof(regPacket))) {
            std::cerr << "Failed to send registration packet" << std::endl;
            return;
        }
        
        uint8_t responseHeader;
        if (!ReceivePacket(&responseHeader, sizeof(responseHeader))) {
            std::cerr << "Failed to receive registration response" << std::endl;
            return;
        }
        
        if (responseHeader == HEADER_GC_REGISTER_PATIENT_SUCCESS) {
            TPacketGCRegisterPatientSuccess response;
            if (ReceivePacket(reinterpret_cast<char*>(&response) + 1, sizeof(response) - 1)) {
                std::cout << "Registration successful! Patient ID: " << response.dwPatientID << std::endl;
                std::cout << response.szMessage << std::endl;
            }
        } else if (responseHeader == HEADER_GC_REGISTER_PATIENT_FAILURE) {
            TPacketGCRegisterPatientFailure response;
            if (ReceivePacket(reinterpret_cast<char*>(&response) + 1, sizeof(response) - 1)) {
                std::cout << "Registration failed: " << response.szErrorMessage << std::endl;
            }
        }
    }
    
    void HandleGetPatientInfo() {
        TPacketCGGetPatientInfo infoPacket;
        infoPacket.bHeader = HEADER_CG_GET_PATIENT_INFO;
        
        std::cout << "Enter patient ID: ";
        std::cin >> infoPacket.dwPatientID;
        std::cin.ignore();
        
        if (!SendPacket(&infoPacket, sizeof(infoPacket))) {
            std::cerr << "Failed to send patient info request" << std::endl;
            return;
        }
        
        uint8_t responseHeader;
        if (!ReceivePacket(&responseHeader, sizeof(responseHeader))) {
            std::cerr << "Failed to receive patient info response" << std::endl;
            return;
        }
        
        if (responseHeader == HEADER_GC_PATIENT_INFO) {
            TPacketGCPatientInfo response;
            if (ReceivePacket(reinterpret_cast<char*>(&response) + 1, sizeof(response) - 1)) {
                const TPatientInfo& info = response.patientInfo;
                std::cout << "\n=== Patient Information ===" << std::endl;
                std::cout << "ID: " << info.dwPatientID << std::endl;
                std::cout << "Name: " << info.szFirstName << " " << info.szLastName << std::endl;
                std::cout << "Date of Birth: " << info.szDateOfBirth << std::endl;
                std::cout << "Gender: " << info.szGender << std::endl;
                std::cout << "Phone: " << info.szPhone << std::endl;
                std::cout << "Email: " << info.szEmail << std::endl;
                std::cout << "Address: " << info.szAddress << std::endl;
                std::cout << "Emergency Contact: " << info.szEmergencyContact << " (" << info.szEmergencyPhone << ")" << std::endl;
                std::cout << "Registration Date: " << info.szRegistrationDate << std::endl;
            }
        } else if (responseHeader == HEADER_GC_ERROR) {
            TPacketGCError response;
            if (ReceivePacket(reinterpret_cast<char*>(&response) + 1, sizeof(response) - 1)) {
                std::cout << "Error: " << response.szErrorMessage << std::endl;
            }
        }
    }
    
    void HandleBookAppointment() {
        TPacketCGBookAppointment appointmentPacket;
        appointmentPacket.bHeader = HEADER_CG_BOOK_APPOINTMENT;
        
        std::cout << "Enter appointment details:" << std::endl;
        std::cout << "Patient ID: ";
        std::cin >> appointmentPacket.dwPatientID;
        std::cin.ignore();
        
        std::cout << "Doctor ID: ";
        std::cin >> appointmentPacket.dwDoctorID;
        std::cin.ignore();
        
        std::cout << "Appointment date (YYYY-MM-DD): ";
        std::cin.getline(appointmentPacket.szAppointmentDate, sizeof(appointmentPacket.szAppointmentDate));
        
        std::cout << "Appointment time (HH:MM): ";
        std::cin.getline(appointmentPacket.szAppointmentTime, sizeof(appointmentPacket.szAppointmentTime));
        
        std::cout << "Reason for visit: ";
        std::cin.getline(appointmentPacket.szReason, sizeof(appointmentPacket.szReason));
        
        if (!SendPacket(&appointmentPacket, sizeof(appointmentPacket))) {
            std::cerr << "Failed to send appointment request" << std::endl;
            return;
        }
        
        uint8_t responseHeader;
        if (!ReceivePacket(&responseHeader, sizeof(responseHeader))) {
            std::cerr << "Failed to receive appointment response" << std::endl;
            return;
        }
        
        if (responseHeader == HEADER_GC_APPOINTMENT_SUCCESS) {
            TPacketGCAppointmentSuccess response;
            if (ReceivePacket(reinterpret_cast<char*>(&response) + 1, sizeof(response) - 1)) {
                std::cout << "Appointment booked successfully! ID: " << response.dwAppointmentID << std::endl;
                std::cout << response.szMessage << std::endl;
            }
        } else if (responseHeader == HEADER_GC_APPOINTMENT_FAILURE) {
            TPacketGCAppointmentFailure response;
            if (ReceivePacket(reinterpret_cast<char*>(&response) + 1, sizeof(response) - 1)) {
                std::cout << "Appointment booking failed: " << response.szErrorMessage << std::endl;
            }
        }
    }
    
    void HandleViewAppointments() {
        TPacketCGGetAppointments appointmentsPacket;
        appointmentsPacket.bHeader = HEADER_CG_GET_APPOINTMENTS;
        
        std::cout << "Enter patient ID: ";
        std::cin >> appointmentsPacket.dwPatientID;
        std::cin.ignore();
        
        std::cout << "Start date (YYYY-MM-DD, or press Enter for all): ";
        std::cin.getline(appointmentsPacket.szStartDate, sizeof(appointmentsPacket.szStartDate));
        
        std::cout << "End date (YYYY-MM-DD, or press Enter for all): ";
        std::cin.getline(appointmentsPacket.szEndDate, sizeof(appointmentsPacket.szEndDate));
        
        if (!SendPacket(&appointmentsPacket, sizeof(appointmentsPacket))) {
            std::cerr << "Failed to send appointments request" << std::endl;
            return;
        }
        
        uint8_t responseHeader;
        if (!ReceivePacket(&responseHeader, sizeof(responseHeader))) {
            std::cerr << "Failed to receive appointments response" << std::endl;
            return;
        }
        
        if (responseHeader == HEADER_GC_APPOINTMENT_LIST) {
            TPacketGCAppointmentList response;
            if (ReceivePacket(reinterpret_cast<char*>(&response) + 1, sizeof(response) - 1)) {
                std::cout << "\n=== Appointments (" << response.wCount << " found) ===" << std::endl;
                
                for (int i = 0; i < response.wCount; i++) {
                    TAppointmentInfo appointment;
                    if (ReceivePacket(&appointment, sizeof(appointment))) {
                        std::cout << "ID: " << appointment.dwAppointmentID << std::endl;
                        std::cout << "Patient: " << appointment.szPatientName << std::endl;
                        std::cout << "Doctor: " << appointment.szDoctorName << std::endl;
                        std::cout << "Date: " << appointment.szAppointmentDate << " at " << appointment.szAppointmentTime << std::endl;
                        std::cout << "Reason: " << appointment.szReason << std::endl;
                        std::cout << "Status: " << appointment.szStatus << std::endl;
                        std::cout << "---" << std::endl;
                    }
                }
            }
        }
    }
    
    void HandleViewDoctors() {
        TPacketCGGetDoctors doctorsPacket;
        doctorsPacket.bHeader = HEADER_CG_GET_DOCTORS;
        
        std::cout << "Enter specialization (or press Enter for all doctors): ";
        std::cin.getline(doctorsPacket.szSpecialization, sizeof(doctorsPacket.szSpecialization));
        
        if (!SendPacket(&doctorsPacket, sizeof(doctorsPacket))) {
            std::cerr << "Failed to send doctors request" << std::endl;
            return;
        }
        
        uint8_t responseHeader;
        if (!ReceivePacket(&responseHeader, sizeof(responseHeader))) {
            std::cerr << "Failed to receive doctors response" << std::endl;
            return;
        }
        
        if (responseHeader == HEADER_GC_DOCTOR_LIST) {
            TPacketGCDoctorList response;
            if (ReceivePacket(reinterpret_cast<char*>(&response) + 1, sizeof(response) - 1)) {
                std::cout << "\n=== Available Doctors (" << response.wCount << " found) ===" << std::endl;
                
                for (int i = 0; i < response.wCount; i++) {
                    TDoctorInfo doctor;
                    if (ReceivePacket(&doctor, sizeof(doctor))) {
                        std::cout << "ID: " << doctor.dwDoctorID << std::endl;
                        std::cout << "Name: Dr. " << doctor.szFirstName << " " << doctor.szLastName << std::endl;
                        std::cout << "Specialization: " << doctor.szSpecialization << std::endl;
                        std::cout << "Office: " << doctor.szOfficeRoom << std::endl;
                        std::cout << "Phone: " << doctor.szPhone << std::endl;
                        std::cout << "Email: " << doctor.szEmail << std::endl;
                        std::cout << "Available: " << (doctor.bAvailable ? "Yes" : "No") << std::endl;
                        std::cout << "---" << std::endl;
                    }
                }
            }
        }
    }
    
    void HandleAddMedicalRecord() {
        if (userType != USER_TYPE_DOCTOR) {
            std::cout << "Only doctors can add medical records." << std::endl;
            return;
        }
        
        TPacketCGAddMedicalRecord recordPacket;
        recordPacket.bHeader = HEADER_CG_ADD_MEDICAL_RECORD;
        
        std::cout << "Enter medical record details:" << std::endl;
        std::cout << "Patient ID: ";
        std::cin >> recordPacket.dwPatientID;
        std::cin.ignore();
        
        recordPacket.dwDoctorID = userID; // Use current logged-in doctor
        
        std::cout << "Diagnosis: ";
        std::cin.getline(recordPacket.szDiagnosis, sizeof(recordPacket.szDiagnosis));
        
        std::cout << "Treatment: ";
        std::cin.getline(recordPacket.szTreatment, sizeof(recordPacket.szTreatment));
        
        std::cout << "Prescription: ";
        std::cin.getline(recordPacket.szPrescription, sizeof(recordPacket.szPrescription));
        
        std::cout << "Notes: ";
        std::cin.getline(recordPacket.szNotes, sizeof(recordPacket.szNotes));
        
        if (!SendPacket(&recordPacket, sizeof(recordPacket))) {
            std::cerr << "Failed to send medical record" << std::endl;
            return;
        }
        
        uint8_t responseHeader;
        if (!ReceivePacket(&responseHeader, sizeof(responseHeader))) {
            std::cerr << "Failed to receive medical record response" << std::endl;
            return;
        }
        
        if (responseHeader == HEADER_GC_MEDICAL_RECORD_SUCCESS) {
            TPacketGCMedicalRecordSuccess response;
            if (ReceivePacket(reinterpret_cast<char*>(&response) + 1, sizeof(response) - 1)) {
                std::cout << "Medical record added successfully! ID: " << response.dwRecordID << std::endl;
                std::cout << response.szMessage << std::endl;
            }
        } else if (responseHeader == HEADER_GC_MEDICAL_RECORD_FAILURE) {
            TPacketGCMedicalRecordFailure response;
            if (ReceivePacket(reinterpret_cast<char*>(&response) + 1, sizeof(response) - 1)) {
                std::cout << "Failed to add medical record: " << response.szErrorMessage << std::endl;
            }
        }
    }
    
    void HandleViewMedicalHistory() {
        TPacketCGGetMedicalHistory historyPacket;
        historyPacket.bHeader = HEADER_CG_GET_MEDICAL_HISTORY;
        
        std::cout << "Enter patient ID: ";
        std::cin >> historyPacket.dwPatientID;
        std::cin.ignore();
        
        std::cout << "Start date (YYYY-MM-DD, or press Enter for all): ";
        std::cin.getline(historyPacket.szStartDate, sizeof(historyPacket.szStartDate));
        
        std::cout << "End date (YYYY-MM-DD, or press Enter for all): ";
        std::cin.getline(historyPacket.szEndDate, sizeof(historyPacket.szEndDate));
        
        if (!SendPacket(&historyPacket, sizeof(historyPacket))) {
            std::cerr << "Failed to send medical history request" << std::endl;
            return;
        }
        
        uint8_t responseHeader;
        if (!ReceivePacket(&responseHeader, sizeof(responseHeader))) {
            std::cerr << "Failed to receive medical history response" << std::endl;
            return;
        }
        
        if (responseHeader == HEADER_GC_MEDICAL_HISTORY) {
            TPacketGCMedicalHistory response;
            if (ReceivePacket(reinterpret_cast<char*>(&response) + 1, sizeof(response) - 1)) {
                std::cout << "\n=== Medical History (" << response.wCount << " records found) ===" << std::endl;
                
                for (int i = 0; i < response.wCount; i++) {
                    TMedicalRecord record;
                    if (ReceivePacket(&record, sizeof(record))) {
                        std::cout << "Record ID: " << record.dwRecordID << std::endl;
                        std::cout << "Date: " << record.szRecordDate << std::endl;
                        std::cout << "Doctor: " << record.szDoctorName << std::endl;
                        std::cout << "Diagnosis: " << record.szDiagnosis << std::endl;
                        std::cout << "Treatment: " << record.szTreatment << std::endl;
                        std::cout << "Prescription: " << record.szPrescription << std::endl;
                        std::cout << "Notes: " << record.szNotes << std::endl;
                        std::cout << "---" << std::endl;
                    }
                }
            }
        }
    }
    
public:
    HospitalClient() : client_fd(-1), connected(false), authenticated(false), userID(0), userType(0) {}
    
    ~HospitalClient() {
        Disconnect();
    }
    
    bool Connect(const std::string& host = "127.0.0.1", int port = 13000) {
        client_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (client_fd == -1) {
            perror("socket");
            return false;
        }
        
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address: " << host << std::endl;
            return false;
        }
        
        if (connect(client_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
            perror("connect");
            return false;
        }
        
        // Handle handshake
        TPacketGCHandshake handshakeIn;
        if (!ReceivePacket(&handshakeIn, sizeof(handshakeIn))) {
            std::cerr << "Failed to receive handshake" << std::endl;
            return false;
        }
        
        if (handshakeIn.bHeader != HEADER_GC_HANDSHAKE) {
            std::cerr << "Invalid handshake header" << std::endl;
            return false;
        }
        
        TPacketCGHandshake handshakeOut;
        handshakeOut.bHeader = HEADER_CG_HANDSHAKE;
        handshakeOut.dwHandshake = handshakeIn.dwHandshake;
        handshakeOut.dwTime = handshakeIn.dwTime;
        handshakeOut.lDelta = 0;
        
        if (!SendPacket(&handshakeOut, sizeof(handshakeOut))) {
            std::cerr << "Failed to send handshake response" << std::endl;
            return false;
        }
        
        connected = true;
        std::cout << "Connected to hospital server at " << host << ":" << port << std::endl;
        return true;
    }
    
    void Disconnect() {
        if (client_fd != -1) {
            close(client_fd);
            client_fd = -1;
        }
        connected = false;
        authenticated = false;
        std::cout << "Disconnected from server" << std::endl;
    }
    
    void Run() {
        if (!connected) {
            std::cerr << "Not connected to server" << std::endl;
            return;
        }
        
        int choice;
        while (connected) {
            DisplayMenu();
            std::cin >> choice;
            std::cin.ignore(); // Clear newline
            
            switch (choice) {
                case 1:
                    HandleLogin();
                    break;
                case 2:
                    HandleRegisterPatient();
                    break;
                case 3:
                    if (authenticated) HandleGetPatientInfo();
                    else std::cout << "Please login first." << std::endl;
                    break;
                case 4:
                    if (authenticated) HandleBookAppointment();
                    else std::cout << "Please login first." << std::endl;
                    break;
                case 5:
                    if (authenticated) HandleViewAppointments();
                    else std::cout << "Please login first." << std::endl;
                    break;
                case 6:
                    HandleViewDoctors();
                    break;
                case 7:
                    if (authenticated) HandleAddMedicalRecord();
                    else std::cout << "Please login first." << std::endl;
                    break;
                case 8:
                    if (authenticated) HandleViewMedicalHistory();
                    else std::cout << "Please login first." << std::endl;
                    break;
                case 9:
                    Disconnect();
                    return;
                default:
                    std::cout << "Invalid choice. Please try again." << std::endl;
                    break;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 13000;
    
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = std::atoi(argv[2]);
    }
    
    HospitalClient client;
    
    if (!client.Connect(host, port)) {
        std::cerr << "Failed to connect to hospital server" << std::endl;
        return 1;
    }
    
    std::cout << "Welcome to Hospital Management System Client!" << std::endl;
    std::cout << "You can register as a new patient without logging in." << std::endl;
    std::cout << "To access other features, you need to login with valid credentials." << std::endl;
    
    client.Run();
    
    return 0;
}