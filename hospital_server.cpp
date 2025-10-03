#include "hospital_packet.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <vector>
#include <map>
#include <algorithm>

// Simple in-memory data structures
struct Patient {
    uint32_t dwPatientID;
    char szName[32];
    char szSurname[32];
    char szTC[11];
    char szPhone[15];
    char szAddress[64];
    uint8_t bAge;
    uint8_t bGender;
};

struct Doctor {
    uint32_t dwDoctorID;
    char szName[32];
    char szSurname[32];
    uint8_t bSpecialization;
    uint8_t bAvailable;
};

struct Appointment {
    uint32_t dwAppointmentID;
    uint32_t dwPatientID;
    uint32_t dwDoctorID;
    uint32_t dwDateTime;
    char szReason[64];
    uint8_t bStatus;
};

struct MedicalRecord {
    uint32_t dwRecordID;
    uint32_t dwPatientID;
    uint32_t dwDateTime;
    char szDoctorName[32];
    char szDiagnosis[64];
    char szTreatment[128];
    char szNotes[128];
};

class HospitalServer {
private:
    std::map<uint32_t, Patient> patients;
    std::map<uint32_t, Doctor> doctors;
    std::map<uint32_t, Appointment> appointments;
    std::map<uint32_t, std::vector<MedicalRecord>> medicalRecords;
    
    uint32_t nextPatientID;
    uint32_t nextDoctorID;
    uint32_t nextAppointmentID;
    uint32_t nextRecordID;
    
    int server_fd;

public:
    HospitalServer() : nextPatientID(1000), nextDoctorID(2000), 
                      nextAppointmentID(3000), nextRecordID(4000) {
        initializeDoctors();
    }
    
    void initializeDoctors() {
        // Add some sample doctors
        Doctor doc1 = {nextDoctorID++, "Dr. Ahmet", "Yılmaz", SPECIALIZATION_CARDIOLOGY, 1};
        Doctor doc2 = {nextDoctorID++, "Dr. Ayşe", "Demir", SPECIALIZATION_NEUROLOGY, 1};
        Doctor doc3 = {nextDoctorID++, "Dr. Mehmet", "Kaya", SPECIALIZATION_ORTHOPEDICS, 1};
        Doctor doc4 = {nextDoctorID++, "Dr. Fatma", "Özkan", SPECIALIZATION_PEDIATRICS, 1};
        Doctor doc5 = {nextDoctorID++, "Dr. Ali", "Çelik", SPECIALIZATION_DERMATOLOGY, 1};
        
        doctors[doc1.dwDoctorID] = doc1;
        doctors[doc2.dwDoctorID] = doc2;
        doctors[doc3.dwDoctorID] = doc3;
        doctors[doc4.dwDoctorID] = doc4;
        doctors[doc5.dwDoctorID] = doc5;
    }
    
    const char* getSpecializationName(uint8_t specialization) {
        switch(specialization) {
            case SPECIALIZATION_GENERAL: return "Genel Pratisyen";
            case SPECIALIZATION_CARDIOLOGY: return "Kardiyoloji";
            case SPECIALIZATION_NEUROLOGY: return "Nöroloji";
            case SPECIALIZATION_ORTHOPEDICS: return "Ortopedi";
            case SPECIALIZATION_PEDIATRICS: return "Pediatri";
            case SPECIALIZATION_DERMATOLOGY: return "Dermatoloji";
            default: return "Bilinmeyen";
        }
    }
    
    bool startServer(int port) {
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

        if (listen(server_fd, 5) == -1) {
            perror("listen");
            return false;
        }

        std::cout << "Hospital server started on port " << port << std::endl;
        return true;
    }
    
    void handleClient(int client_fd) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        getpeername(client_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        
        std::cout << "Client connected: " << inet_ntoa(client_addr.sin_addr) << std::endl;

        // Handshake - Server waits for client handshake first
        TPacketCGHandshake in_packet{};
        ssize_t received = recv(client_fd, &in_packet, sizeof(in_packet), 0);
        if (received != sizeof(in_packet)) {
            std::cerr << "Invalid handshake size" << std::endl;
            close(client_fd);
            return;
        }

        if (in_packet.bHeader != HEADER_CG_HANDSHAKE) {
            std::cerr << "Invalid handshake header" << std::endl;
            close(client_fd);
            return;
        }

        // Send handshake response
        TPacketGCHandshake out_packet{};
        out_packet.bHeader = HEADER_GC_HANDSHAKE;
        out_packet.dwHandshake = in_packet.dwHandshake;
        out_packet.dwTime = static_cast<uint32_t>(time(nullptr));
        out_packet.lDelta = 0;

        if (send(client_fd, &out_packet, sizeof(out_packet), 0) != sizeof(out_packet)) {
            std::cerr << "Failed to send handshake response" << std::endl;
            close(client_fd);
            return;
        }

        std::cout << "Handshake successful" << std::endl;

        // Main communication loop
        while (true) {
            uint8_t header;
            ssize_t bytes_received = recv(client_fd, &header, sizeof(header), 0);
            
            if (bytes_received <= 0) {
                std::cout << "Client disconnected" << std::endl;
                break;
            }
            
            switch (header) {
                case HEADER_CG_PATIENT_REGISTER:
                    handlePatientRegister(client_fd);
                    break;
                case HEADER_CG_PATIENT_LOGIN:
                    handlePatientLogin(client_fd);
                    break;
                case HEADER_CG_APPOINTMENT_REQUEST:
                    handleAppointmentRequest(client_fd);
                    break;
                case HEADER_CG_APPOINTMENT_LIST:
                    handleAppointmentList(client_fd);
                    break;
                case HEADER_CG_DOCTOR_LIST:
                    handleDoctorList(client_fd);
                    break;
                case HEADER_CG_MEDICAL_RECORD:
                    handleMedicalRecord(client_fd);
                    break;
                default:
                    std::cerr << "Unknown packet header: 0x" << std::hex << (int)header << std::endl;
                    break;
            }
        }
        
        close(client_fd);
    }
    
    void handlePatientRegister(int client_fd) {
        TPacketCGPatientRegister packet;
        ssize_t received = recv(client_fd, reinterpret_cast<char*>(&packet) + 1, sizeof(packet) - 1, 0);
        
        if (received != sizeof(packet) - 1) {
            sendErrorResponse(client_fd, RESPONSE_ERROR_INVALID_DATA, "Invalid packet size");
            return;
        }
        
        // Check if patient already exists
        for (const auto& pair : patients) {
            if (strcmp(pair.second.szTC, packet.szTC) == 0) {
                sendErrorResponse(client_fd, RESPONSE_ERROR_INVALID_DATA, "Patient already exists");
                return;
            }
        }
        
        // Create new patient
        Patient newPatient;
        newPatient.dwPatientID = nextPatientID++;
        strncpy(newPatient.szName, packet.szName, sizeof(newPatient.szName) - 1);
        strncpy(newPatient.szSurname, packet.szSurname, sizeof(newPatient.szSurname) - 1);
        strncpy(newPatient.szTC, packet.szTC, sizeof(newPatient.szTC) - 1);
        strncpy(newPatient.szPhone, packet.szPhone, sizeof(newPatient.szPhone) - 1);
        strncpy(newPatient.szAddress, packet.szAddress, sizeof(newPatient.szAddress) - 1);
        newPatient.bAge = packet.bAge;
        newPatient.bGender = packet.bGender;
        
        patients[newPatient.dwPatientID] = newPatient;
        
        // Send response
        TPacketGCPatientRegister response{};
        response.bHeader = HEADER_GC_PATIENT_REGISTER;
        response.bResponseCode = RESPONSE_SUCCESS;
        response.dwPatientID = newPatient.dwPatientID;
        
        send(client_fd, &response, sizeof(response), 0);
        std::cout << "Patient registered: " << newPatient.szName << " " << newPatient.szSurname 
                  << " (ID: " << newPatient.dwPatientID << ")" << std::endl;
    }
    
    void handlePatientLogin(int client_fd) {
        TPacketCGPatientLogin packet;
        ssize_t received = recv(client_fd, reinterpret_cast<char*>(&packet) + 1, sizeof(packet) - 1, 0);
        
        if (received != sizeof(packet) - 1) {
            sendErrorResponse(client_fd, RESPONSE_ERROR_INVALID_DATA, "Invalid packet size");
            return;
        }
        
        // Find patient by TC
        Patient* foundPatient = nullptr;
        for (auto& pair : patients) {
            if (strcmp(pair.second.szTC, packet.szTC) == 0) {
                foundPatient = &pair.second;
                break;
            }
        }
        
        if (!foundPatient) {
            sendErrorResponse(client_fd, RESPONSE_ERROR_PATIENT_NOT_FOUND, "Patient not found");
            return;
        }
        
        // Send response
        TPacketGCPatientLogin response{};
        response.bHeader = HEADER_GC_PATIENT_LOGIN;
        response.bResponseCode = RESPONSE_SUCCESS;
        response.dwPatientID = foundPatient->dwPatientID;
        strncpy(response.szName, foundPatient->szName, sizeof(response.szName) - 1);
        strncpy(response.szSurname, foundPatient->szSurname, sizeof(response.szSurname) - 1);
        
        send(client_fd, &response, sizeof(response), 0);
        std::cout << "Patient login: " << foundPatient->szName << " " << foundPatient->szSurname << std::endl;
    }
    
    void handleAppointmentRequest(int client_fd) {
        TPacketCGAppointmentRequest packet;
        ssize_t received = recv(client_fd, reinterpret_cast<char*>(&packet) + 1, sizeof(packet) - 1, 0);
        
        if (received != sizeof(packet) - 1) {
            sendErrorResponse(client_fd, RESPONSE_ERROR_INVALID_DATA, "Invalid packet size");
            return;
        }
        
        // Check if patient exists
        if (patients.find(packet.dwPatientID) == patients.end()) {
            sendErrorResponse(client_fd, RESPONSE_ERROR_PATIENT_NOT_FOUND, "Patient not found");
            return;
        }
        
        // Check if doctor exists and is available
        if (doctors.find(packet.dwDoctorID) == doctors.end() || !doctors[packet.dwDoctorID].bAvailable) {
            sendErrorResponse(client_fd, RESPONSE_ERROR_DOCTOR_NOT_AVAILABLE, "Doctor not available");
            return;
        }
        
        // Check for appointment conflicts
        for (const auto& pair : appointments) {
            if (pair.second.dwDoctorID == packet.dwDoctorID && 
                pair.second.dwDateTime == packet.dwDateTime &&
                pair.second.bStatus != 3) { // Not cancelled
                sendErrorResponse(client_fd, RESPONSE_ERROR_APPOINTMENT_CONFLICT, "Appointment time conflict");
                return;
            }
        }
        
        // Create appointment
        Appointment newAppointment;
        newAppointment.dwAppointmentID = nextAppointmentID++;
        newAppointment.dwPatientID = packet.dwPatientID;
        newAppointment.dwDoctorID = packet.dwDoctorID;
        newAppointment.dwDateTime = packet.dwDateTime;
        strncpy(newAppointment.szReason, packet.szReason, sizeof(newAppointment.szReason) - 1);
        newAppointment.bStatus = 0; // Pending
        
        appointments[newAppointment.dwAppointmentID] = newAppointment;
        
        // Send response
        TPacketGCAppointmentRequest response{};
        response.bHeader = HEADER_GC_APPOINTMENT_REQUEST;
        response.bResponseCode = RESPONSE_SUCCESS;
        response.dwAppointmentID = newAppointment.dwAppointmentID;
        
        send(client_fd, &response, sizeof(response), 0);
        std::cout << "Appointment created: ID " << newAppointment.dwAppointmentID 
                  << " for patient " << packet.dwPatientID << std::endl;
    }
    
    void handleAppointmentList(int client_fd) {
        TPacketCGAppointmentList packet;
        ssize_t received = recv(client_fd, reinterpret_cast<char*>(&packet) + 1, sizeof(packet) - 1, 0);
        
        if (received != sizeof(packet) - 1) {
            sendErrorResponse(client_fd, RESPONSE_ERROR_INVALID_DATA, "Invalid packet size");
            return;
        }
        
        // Find appointments for this patient
        std::vector<AppointmentInfo> patientAppointments;
        for (const auto& pair : appointments) {
            if (pair.second.dwPatientID == packet.dwPatientID) {
                AppointmentInfo info;
                info.dwAppointmentID = pair.second.dwAppointmentID;
                info.dwDoctorID = pair.second.dwDoctorID;
                info.dwDateTime = pair.second.dwDateTime;
                info.bStatus = pair.second.bStatus;
                strncpy(info.szReason, pair.second.szReason, sizeof(info.szReason) - 1);
                
                // Get doctor info
                if (doctors.find(pair.second.dwDoctorID) != doctors.end()) {
                    const Doctor& doctor = doctors[pair.second.dwDoctorID];
                    snprintf(info.szDoctorName, sizeof(info.szDoctorName), "%s %s", 
                            doctor.szName, doctor.szSurname);
                    strncpy(info.szSpecialization, getSpecializationName(doctor.bSpecialization), 
                           sizeof(info.szSpecialization) - 1);
                }
                
                patientAppointments.push_back(info);
            }
        }
        
        // Send response
        TPacketGCAppointmentList response{};
        response.bHeader = HEADER_GC_APPOINTMENT_LIST;
        response.bResponseCode = RESPONSE_SUCCESS;
        response.bAppointmentCount = std::min(static_cast<uint8_t>(patientAppointments.size()), 
                                            static_cast<uint8_t>(10));
        
        for (int i = 0; i < response.bAppointmentCount; i++) {
            response.appointments[i] = patientAppointments[i];
        }
        
        send(client_fd, &response, sizeof(response), 0);
        std::cout << "Sent " << (int)response.bAppointmentCount << " appointments for patient " 
                  << packet.dwPatientID << std::endl;
    }
    
    void handleDoctorList(int client_fd) {
        TPacketCGDoctorList packet;
        ssize_t received = recv(client_fd, reinterpret_cast<char*>(&packet) + 1, sizeof(packet) - 1, 0);
        
        if (received != sizeof(packet) - 1) {
            sendErrorResponse(client_fd, RESPONSE_ERROR_INVALID_DATA, "Invalid packet size");
            return;
        }
        
        // Collect doctors
        std::vector<DoctorInfo> doctorList;
        for (const auto& pair : doctors) {
            if (packet.bSpecialization == 0 || pair.second.bSpecialization == packet.bSpecialization) {
                DoctorInfo info;
                info.dwDoctorID = pair.second.dwDoctorID;
                strncpy(info.szName, pair.second.szName, sizeof(info.szName) - 1);
                strncpy(info.szSurname, pair.second.szSurname, sizeof(info.szSurname) - 1);
                info.bSpecialization = pair.second.bSpecialization;
                strncpy(info.szSpecializationName, getSpecializationName(pair.second.bSpecialization), 
                       sizeof(info.szSpecializationName) - 1);
                info.bAvailable = pair.second.bAvailable;
                
                doctorList.push_back(info);
            }
        }
        
        // Send response
        TPacketGCDoctorList response{};
        response.bHeader = HEADER_GC_DOCTOR_LIST;
        response.bResponseCode = RESPONSE_SUCCESS;
        response.bDoctorCount = std::min(static_cast<uint8_t>(doctorList.size()), 
                                       static_cast<uint8_t>(20));
        
        for (int i = 0; i < response.bDoctorCount; i++) {
            response.doctors[i] = doctorList[i];
        }
        
        send(client_fd, &response, sizeof(response), 0);
        std::cout << "Sent " << (int)response.bDoctorCount << " doctors" << std::endl;
    }
    
    void handleMedicalRecord(int client_fd) {
        TPacketCGMedicalRecord packet;
        ssize_t received = recv(client_fd, reinterpret_cast<char*>(&packet) + 1, sizeof(packet) - 1, 0);
        
        if (received != sizeof(packet) - 1) {
            sendErrorResponse(client_fd, RESPONSE_ERROR_INVALID_DATA, "Invalid packet size");
            return;
        }
        
        // Find medical records for this patient
        std::vector<MedicalRecordInfo> records;
        if (medicalRecords.find(packet.dwPatientID) != medicalRecords.end()) {
            for (const auto& record : medicalRecords[packet.dwPatientID]) {
                MedicalRecordInfo info;
                info.dwRecordID = record.dwRecordID;
                info.dwDateTime = record.dwDateTime;
                strncpy(info.szDoctorName, record.szDoctorName, sizeof(info.szDoctorName) - 1);
                strncpy(info.szDiagnosis, record.szDiagnosis, sizeof(info.szDiagnosis) - 1);
                strncpy(info.szTreatment, record.szTreatment, sizeof(info.szTreatment) - 1);
                strncpy(info.szNotes, record.szNotes, sizeof(info.szNotes) - 1);
                
                records.push_back(info);
            }
        }
        
        // Send response
        TPacketGCMedicalRecord response{};
        response.bHeader = HEADER_GC_MEDICAL_RECORD;
        response.bResponseCode = RESPONSE_SUCCESS;
        response.bRecordCount = std::min(static_cast<uint8_t>(records.size()), 
                                        static_cast<uint8_t>(10));
        
        for (int i = 0; i < response.bRecordCount; i++) {
            response.records[i] = records[i];
        }
        
        send(client_fd, &response, sizeof(response), 0);
        std::cout << "Sent " << (int)response.bRecordCount << " medical records for patient " 
                  << packet.dwPatientID << std::endl;
    }
    
    void sendErrorResponse(int client_fd, uint8_t errorCode, const char* message) {
        TPacketGCResponse response{};
        response.bHeader = HEADER_GC_ERROR;
        response.bResponseCode = errorCode;
        strncpy(response.szMessage, message, sizeof(response.szMessage) - 1);
        
        send(client_fd, &response, sizeof(response), 0);
    }
    
    void run() {
        while (true) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
            if (client_fd == -1) {
                perror("accept");
                continue;
            }
            
            handleClient(client_fd);
        }
    }
    
    ~HospitalServer() {
        if (server_fd != -1) {
            close(server_fd);
        }
    }
};

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));
    
    HospitalServer server;
    
    if (!server.startServer(13000)) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    std::cout << "Hospital Management System Server" << std::endl;
    std::cout << "Available commands:" << std::endl;
    std::cout << "- Patient Registration" << std::endl;
    std::cout << "- Patient Login" << std::endl;
    std::cout << "- Appointment Booking" << std::endl;
    std::cout << "- Doctor List" << std::endl;
    std::cout << "- Medical Records" << std::endl;
    
    server.run();
    
    return 0;
}