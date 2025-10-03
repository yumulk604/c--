#include "packet.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

// Simple text-based database helper functions
class HospitalDatabase {
private:
    std::vector<std::string> patients;
    std::vector<std::string> doctors;
    std::vector<std::string> appointments;
    std::vector<std::string> medical_records;

    void loadPatients() {
        std::ifstream file("patients.txt");
        std::string line;
        while (std::getline(file, line)) {
            patients.push_back(line);
        }
    }

    void savePatients() {
        std::ofstream file("patients.txt");
        for (const auto& patient : patients) {
            file << patient << std::endl;
        }
    }

    void loadDoctors() {
        std::ifstream file("doctors.txt");
        std::string line;
        while (std::getline(file, line)) {
            doctors.push_back(line);
        }
    }

    void saveDoctors() {
        std::ofstream file("doctors.txt");
        for (const auto& doctor : doctors) {
            file << doctor << std::endl;
        }
    }

public:
    HospitalDatabase() {
        loadPatients();
        loadDoctors();
    }

    ~HospitalDatabase() {
        savePatients();
        saveDoctors();
    }

    bool init() {
        std::cout << "Hospital database initialized (text-based)" << std::endl;
        return true;
    }

    bool authenticateUser(const std::string& username, const std::string& password, uint32_t& userId, std::string& userType) {
        // Simple authentication - for demo purposes only
        // In production, use proper password hashing
        if (username == "admin" && password == "admin") {
            userId = 1;
            userType = "admin";
            return true;
        } else if (username == "doctor" && password == "doctor") {
            userId = 2;
            userType = "doctor";
            return true;
        } else if (username == "patient" && password == "patient") {
            userId = 3;
            userType = "patient";
            return true;
        }
        return false;
    }

    bool registerPatient(const std::string& name, const std::string& surname, const std::string& tcNumber,
                        const std::string& birthDate, const std::string& phone, const std::string& address, char gender) {
        // Check if patient already exists
        for (const auto& patient : patients) {
            if (patient.find(tcNumber) != std::string::npos) {
                return false; // Patient already exists
            }
        }

        // Create patient record
        std::stringstream ss;
        ss << patients.size() + 1 << "|" << name << "|" << surname << "|" << tcNumber << "|"
           << birthDate << "|" << phone << "|" << address << "|" << gender << "|"
           << time(nullptr);

        patients.push_back(ss.str());
        return true;
    }

    bool getPatientsBySearch(const std::string& searchTerm, std::vector<TPacketSCPatientData>& patientList) {
        for (const auto& patientStr : patients) {
            std::stringstream ss(patientStr);
            std::string token;
            std::vector<std::string> tokens;

            while (std::getline(ss, token, '|')) {
                tokens.push_back(token);
            }

            if (tokens.size() >= 9) {
                std::string fullName = tokens[1] + " " + tokens[2];
                if (fullName.find(searchTerm) != std::string::npos ||
                    tokens[3].find(searchTerm) != std::string::npos) {

                    TPacketSCPatientData patient;
                    patient.dwPatientID = std::stoi(tokens[0]);
                    strncpy(patient.szName, tokens[1].c_str(), sizeof(patient.szName) - 1);
                    strncpy(patient.szSurname, tokens[2].c_str(), sizeof(patient.szSurname) - 1);
                    strncpy(patient.szTCNumber, tokens[3].c_str(), sizeof(patient.szTCNumber) - 1);
                    strncpy(patient.szBirthDate, tokens[4].c_str(), sizeof(patient.szBirthDate) - 1);
                    strncpy(patient.szPhone, tokens[5].c_str(), sizeof(patient.szPhone) - 1);
                    strncpy(patient.szAddress, tokens[6].c_str(), sizeof(patient.szAddress) - 1);
                    patient.szGender = tokens[7][0];
                    strncpy(patient.szRegisterDate, tokens[8].c_str(), sizeof(patient.szRegisterDate) - 1);

                    patientList.push_back(patient);
                }
            }
        }
        return true;
    }

    bool bookAppointment(uint32_t patientId, uint32_t doctorId, const std::string& appointmentDate, const std::string& notes) {
        std::stringstream ss;
        ss << appointments.size() + 1 << "|" << patientId << "|" << doctorId << "|"
           << appointmentDate << "|" << notes << "|confirmed";

        appointments.push_back(ss.str());
        return true;
    }

    bool registerDoctor(const std::string& name, const std::string& surname, const std::string& specialty,
                       const std::string& phone, const std::string& email) {
        std::stringstream ss;
        ss << doctors.size() + 1 << "|" << name << "|" << surname << "|" << specialty << "|"
           << phone << "|" << email;

        doctors.push_back(ss.str());
        return true;
    }

    bool getAllDoctors(std::vector<std::string>& doctorList) {
        for (const auto& doctorStr : doctors) {
            std::stringstream ss(doctorStr);
            std::string token;
            std::vector<std::string> tokens;

            while (std::getline(ss, token, '|')) {
                tokens.push_back(token);
            }

            if (tokens.size() >= 4) {
                std::string doctorInfo = tokens[1] + " " + tokens[2] + " - " + tokens[3];
                doctorList.push_back(doctorInfo);
            }
        }
        return true;
    }

    bool addMedicalRecord(uint32_t patientId, uint32_t doctorId, const std::string& diagnosis,
                         const std::string& treatment, const std::string& medications, const std::string& notes) {
        std::stringstream ss;
        ss << medical_records.size() + 1 << "|" << patientId << "|" << doctorId << "|"
           << diagnosis << "|" << treatment << "|" << medications << "|" << notes << "|"
           << time(nullptr);

        medical_records.push_back(ss.str());
        return true;
    }
};

uint32_t get_dword_time() {
    return static_cast<uint32_t>(time(nullptr));
}

void sendError(int client_fd, const std::string& errorMessage) {
    TPacketSCError errorPacket;
    errorPacket.bHeader = HEADER_SC_ERROR;
    strncpy(errorPacket.szErrorMessage, errorMessage.c_str(), sizeof(errorPacket.szErrorMessage) - 1);
    errorPacket.szErrorMessage[sizeof(errorPacket.szErrorMessage) - 1] = '\0';

    send(client_fd, &errorPacket, sizeof(errorPacket), 0);
}

void handleLogin(int client_fd, const TPacketCSLogin& loginPacket, HospitalDatabase& db) {
    std::string username(loginPacket.szUsername);
    std::string password(loginPacket.szPassword);

    uint32_t userId;
    std::string userType;

    TPacketSCLoginResponse response;
    response.bHeader = HEADER_SC_LOGIN_RESPONSE;

    if (db.authenticateUser(username, password, userId, userType)) {
        response.bSuccess = 1;
        response.dwUserID = userId;
        strncpy(response.szUserType, userType.c_str(), sizeof(response.szUserType) - 1);
        response.szUserType[sizeof(response.szUserType) - 1] = '\0';
        std::cout << "User logged in: " << username << " (Type: " << userType << ")" << std::endl;
    } else {
        response.bSuccess = 0;
        response.dwUserID = 0;
        strcpy(response.szUserType, "unknown");
        std::cout << "Login failed for user: " << username << std::endl;
    }

    send(client_fd, &response, sizeof(response), 0);
}

void handlePatientRegistration(int client_fd, const TPacketCSPatientRegister& patientPacket, HospitalDatabase& db) {
    std::string name(patientPacket.szName);
    std::string surname(patientPacket.szSurname);
    std::string tcNumber(patientPacket.szTCNumber);
    std::string birthDate(patientPacket.szBirthDate);

    if (db.registerPatient(name, surname, tcNumber, birthDate, patientPacket.szPhone,
                          patientPacket.szAddress, patientPacket.szGender)) {
        std::cout << "Patient registered: " << name << " " << surname << std::endl;
        // Send success confirmation
        const char* successMsg = "Patient registered successfully";
        send(client_fd, successMsg, strlen(successMsg), 0);
    } else {
        sendError(client_fd, "Failed to register patient");
    }
}

void handlePatientSearch(int client_fd, const TPacketCSPatientSearch& searchPacket, HospitalDatabase& db) {
    std::string searchTerm(searchPacket.szSearchTerm);
    std::vector<TPacketSCPatientData> patients;

    if (db.getPatientsBySearch(searchTerm, patients)) {
        TPacketSCPatientList patientList;
        patientList.bHeader = HEADER_SC_PATIENT_LIST;
        patientList.dwCount = patients.size();

        send(client_fd, &patientList, sizeof(patientList), 0);

        for (const auto& patient : patients) {
            send(client_fd, &patient, sizeof(patient), 0);
        }

        std::cout << "Patient search completed for: " << searchTerm << " (Found: " << patients.size() << ")" << std::endl;
    } else {
        sendError(client_fd, "Failed to search patients");
    }
}

void handleDoctorRegistration(int client_fd, const TPacketCSRegisterDoctor& doctorPacket, HospitalDatabase& db) {
    std::string name(doctorPacket.szName);
    std::string surname(doctorPacket.szSurname);
    std::string specialty(doctorPacket.szSpecialty);

    if (db.registerDoctor(name, surname, specialty, doctorPacket.szPhone, doctorPacket.szEmail)) {
        std::cout << "Doctor registered: " << name << " " << surname << " (" << specialty << ")" << std::endl;
        const char* successMsg = "Doctor registered successfully";
        send(client_fd, successMsg, strlen(successMsg), 0);
    } else {
        sendError(client_fd, "Failed to register doctor");
    }
}

void handleListDoctors(int client_fd, HospitalDatabase& db) {
    std::vector<std::string> doctors;

    if (db.getAllDoctors(doctors)) {
        // Send doctor count first
        uint32_t count = doctors.size();
        send(client_fd, &count, sizeof(count), 0);

        // Send each doctor info
        for (const auto& doctor : doctors) {
            uint32_t length = doctor.length();
            send(client_fd, &length, sizeof(length), 0);
            send(client_fd, doctor.c_str(), length, 0);
        }

        std::cout << "Doctor list sent (Count: " << doctors.size() << ")" << std::endl;
    } else {
        sendError(client_fd, "Failed to retrieve doctor list");
    }
}

void handleBookAppointment(int client_fd, const TPacketCSBookAppointment& appointmentPacket, HospitalDatabase& db) {
    std::string appointmentDate(appointmentPacket.szAppointmentDate);

    if (db.bookAppointment(appointmentPacket.dwPatientID, appointmentPacket.dwDoctorID,
                          appointmentDate, appointmentPacket.szNotes)) {
        std::cout << "Appointment booked for Patient ID: " << appointmentPacket.dwPatientID
                  << " with Doctor ID: " << appointmentPacket.dwDoctorID << std::endl;

        TPacketSCAppointmentConfirm confirm;
        confirm.bHeader = HEADER_SC_APPOINTMENT_CONFIRM;
        confirm.dwAppointmentID = 0;  // Will be set by database in real implementation
        confirm.dwPatientID = appointmentPacket.dwPatientID;
        confirm.dwDoctorID = appointmentPacket.dwDoctorID;
        strncpy(confirm.szAppointmentDate, appointmentDate.c_str(), sizeof(confirm.szAppointmentDate) - 1);
        strncpy(confirm.szNotes, appointmentPacket.szNotes, sizeof(confirm.szNotes) - 1);
        strcpy(confirm.szStatus, "confirmed");

        send(client_fd, &confirm, sizeof(confirm), 0);
    } else {
        sendError(client_fd, "Failed to book appointment");
    }
}

void handleAddMedicalRecord(int client_fd, const TPacketCSAddMedicalRecord& recordPacket, HospitalDatabase& db) {
    if (db.addMedicalRecord(recordPacket.dwPatientID, recordPacket.dwDoctorID,
                           recordPacket.szDiagnosis, recordPacket.szTreatment,
                           recordPacket.szMedications, recordPacket.szNotes)) {
        std::cout << "Medical record added for Patient ID: " << recordPacket.dwPatientID << std::endl;
        const char* successMsg = "Medical record added successfully";
        send(client_fd, successMsg, strlen(successMsg), 0);
    } else {
        sendError(client_fd, "Failed to add medical record");
    }
}

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));

    // Initialize database
    HospitalDatabase db;
    if (!db.init()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return 1;
    }

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

    std::cout << "Hospital Management Server started on port 13000" << std::endl;
    std::cout << "Features: Patient Registration, Search, Appointments, Medical Records" << std::endl;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        std::cout << "Client connected: " << inet_ntoa(client_addr.sin_addr) << std::endl;

        // Handshake for compatibility
        uint32_t handshake = static_cast<uint32_t>(rand());
        TPacketGCHandshake out_packet{};
        out_packet.bHeader = HEADER_GC_HANDSHAKE;
        out_packet.dwHandshake = handshake;
        out_packet.dwTime = get_dword_time();
        out_packet.lDelta = 0;

        if (send(client_fd, &out_packet, sizeof(out_packet), 0) != sizeof(out_packet)) {
            std::cerr << "Failed to send handshake" << std::endl;
            close(client_fd);
            continue;
        }

        TPacketCGHandshake in_packet{};
        ssize_t received = recv(client_fd, &in_packet, sizeof(in_packet), 0);
        if (received != sizeof(in_packet)) {
            std::cerr << "Invalid handshake size" << std::endl;
            close(client_fd);
            continue;
        }

        if (in_packet.bHeader != HEADER_CG_HANDSHAKE || in_packet.dwHandshake != handshake) {
            std::cerr << "Handshake mismatch" << std::endl;
            close(client_fd);
            continue;
        }

        std::cout << "Handshake successful" << std::endl;

        // Main client loop
        bool clientConnected = true;
        while (clientConnected) {
            uint8_t header;
            received = recv(client_fd, &header, sizeof(header), 0);

            if (received <= 0) {
                std::cout << "Client disconnected" << std::endl;
                break;
            }

            switch (header) {
                case HEADER_CS_LOGIN: {
                    TPacketCSLogin loginPacket;
                    if (recv(client_fd, &loginPacket.szUsername, sizeof(loginPacket) - 1, 0) == sizeof(loginPacket) - 1) {
                        handleLogin(client_fd, loginPacket, db);
                    }
                    break;
                }

                case HEADER_CS_PATIENT_REGISTER: {
                    TPacketCSPatientRegister patientPacket;
                    if (recv(client_fd, &patientPacket.szName, sizeof(patientPacket) - 1, 0) == sizeof(patientPacket) - 1) {
                        handlePatientRegistration(client_fd, patientPacket, db);
                    }
                    break;
                }

                case HEADER_CS_PATIENT_SEARCH: {
                    TPacketCSPatientSearch searchPacket;
                    if (recv(client_fd, &searchPacket.szSearchTerm, sizeof(searchPacket) - 1, 0) == sizeof(searchPacket) - 1) {
                        handlePatientSearch(client_fd, searchPacket, db);
                    }
                    break;
                }

                case HEADER_CS_DOCTOR_REGISTER: {
                    TPacketCSRegisterDoctor doctorPacket;
                    if (recv(client_fd, &doctorPacket.szName, sizeof(doctorPacket) - 1, 0) == sizeof(doctorPacket) - 1) {
                        handleDoctorRegistration(client_fd, doctorPacket, db);
                    }
                    break;
                }

                case HEADER_CS_DOCTOR_LIST: {
                    handleListDoctors(client_fd, db);
                    break;
                }

                case HEADER_CS_APPOINTMENT_BOOK: {
                    TPacketCSBookAppointment appointmentPacket;
                    if (recv(client_fd, &appointmentPacket.szAppointmentDate, sizeof(appointmentPacket) - 1, 0) == sizeof(appointmentPacket) - 1) {
                        handleBookAppointment(client_fd, appointmentPacket, db);
                    }
                    break;
                }

                case HEADER_CS_MEDICAL_RECORD_ADD: {
                    TPacketCSAddMedicalRecord recordPacket;
                    if (recv(client_fd, &recordPacket.szDiagnosis, sizeof(recordPacket) - 1, 0) == sizeof(recordPacket) - 1) {
                        handleAddMedicalRecord(client_fd, recordPacket, db);
                    }
                    break;
                }

                case HEADER_CS_LOGOUT: {
                    std::cout << "Client logged out" << std::endl;
                    clientConnected = false;
                    break;
                }

                default: {
                    std::cout << "Unknown packet header: " << static_cast<int>(header) << std::endl;
                    sendError(client_fd, "Unknown command");
                    break;
                }
            }
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}