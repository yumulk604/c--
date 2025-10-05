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
    int sock_fd;
    sockaddr_in server_addr;

public:
    HospitalClient() : sock_fd(-1) {
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(13000);
        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    }

    ~HospitalClient() {
        if (sock_fd != -1) {
            close(sock_fd);
        }
    }

    bool connect_to_server() {
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd == -1) {
            perror("socket");
            return false;
        }

        if (connect(sock_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
            perror("connect");
            return false;
        }

        std::cout << "Connected to hospital server\n";
        return true;
    }

    void register_patient() {
        TPacketPatientRegister packet;
        
        std::cout << "\n=== Patient Registration ===\n";
        std::cout << "Enter patient name: ";
        std::string name;
        std::getline(std::cin, name);
        
        std::cout << "Enter patient surname: ";
        std::string surname;
        std::getline(std::cin, surname);
        
        std::cout << "Enter phone number: ";
        std::string phone;
        std::getline(std::cin, phone);
        
        std::cout << "Enter address: ";
        std::string address;
        std::getline(std::cin, address);
        
        std::cout << "Enter age: ";
        int age;
        std::cin >> age;
        std::cin.ignore();
        
        std::cout << "Enter gender (M/F): ";
        char gender;
        std::cin >> gender;
        std::cin.ignore();
        
        std::cout << "Enter blood type: ";
        std::string blood_type;
        std::getline(std::cin, blood_type);
        
        init_patient_register(packet, name.c_str(), surname.c_str(), 
                            phone.c_str(), address.c_str(), 
                            static_cast<uint8_t>(age), gender, blood_type.c_str());
        
        // Send header first
        uint8_t header = HEADER_PATIENT_REGISTER;
        send(sock_fd, &header, sizeof(header), 0);
        
        // Send packet
        send(sock_fd, &packet, sizeof(packet), 0);
        
        // Receive response
        TPacketResponse response;
        recv(sock_fd, &response, sizeof(response), 0);
        
        std::cout << "Server response: " << response.message << std::endl;
    }

    void search_patient() {
        std::cout << "\n=== Patient Search ===\n";
        std::cout << "Search by:\n";
        std::cout << "1. Name\n";
        std::cout << "2. Phone\n";
        std::cout << "3. ID\n";
        std::cout << "Enter choice (1-3): ";
        
        int choice;
        std::cin >> choice;
        std::cin.ignore();
        
        std::string search_term;
        std::cout << "Enter search term: ";
        std::getline(std::cin, search_term);
        
        TPacketPatientSearch packet;
        init_patient_search(packet, search_term.c_str(), static_cast<uint8_t>(choice - 1));
        
        // Send header first
        uint8_t header = HEADER_PATIENT_SEARCH;
        send(sock_fd, &header, sizeof(header), 0);
        
        // Send packet
        send(sock_fd, &packet, sizeof(packet), 0);
        
        // Receive response
        TPacketPatientInfo info;
        ssize_t received = recv(sock_fd, &info, sizeof(info), 0);
        
        if (received == sizeof(info) && info.bHeader == HEADER_PATIENT_INFO) {
            std::cout << "\n=== Patient Information ===\n";
            std::cout << "ID: " << info.patient_id << std::endl;
            std::cout << "Name: " << info.name << " " << info.surname << std::endl;
            std::cout << "Phone: " << info.phone << std::endl;
            std::cout << "Address: " << info.address << std::endl;
            std::cout << "Age: " << static_cast<int>(info.age) << std::endl;
            std::cout << "Gender: " << info.gender << std::endl;
            std::cout << "Blood Type: " << info.blood_type << std::endl;
            std::cout << "Registration Date: " << info.registration_date << std::endl;
        } else {
            TPacketResponse response;
            recv(sock_fd, &response, sizeof(response), 0);
            std::cout << "Error: " << response.message << std::endl;
        }
    }

    void book_appointment() {
        std::cout << "\n=== Book Appointment ===\n";
        
        std::cout << "Enter patient ID: ";
        uint32_t patient_id;
        std::cin >> patient_id;
        
        std::cout << "Enter doctor ID: ";
        uint32_t doctor_id;
        std::cin >> doctor_id;
        
        std::cout << "Enter day of week (0=Monday, 6=Sunday): ";
        int day;
        std::cin >> day;
        
        std::cout << "Enter hour (9-16): ";
        int hour;
        std::cin >> hour;
        std::cin.ignore();
        
        std::cout << "Enter reason for visit: ";
        std::string reason;
        std::getline(std::cin, reason);
        
        TPacketAppointmentBook packet;
        packet.bHeader = HEADER_APPOINTMENT_BOOK;
        packet.patient_id = patient_id;
        packet.doctor_id = doctor_id;
        packet.day_of_week = static_cast<uint8_t>(day);
        packet.hour = static_cast<uint8_t>(hour);
        strncpy(packet.reason, reason.c_str(), sizeof(packet.reason) - 1);
        packet.reason[sizeof(packet.reason) - 1] = '\0';
        
        // Send header first
        uint8_t header = HEADER_APPOINTMENT_BOOK;
        send(sock_fd, &header, sizeof(header), 0);
        
        // Send packet
        send(sock_fd, &packet, sizeof(packet), 0);
        
        // Receive response
        TPacketResponse response;
        recv(sock_fd, &response, sizeof(response), 0);
        
        std::cout << "Server response: " << response.message << std::endl;
    }

    void add_medical_record() {
        std::cout << "\n=== Add Medical Record ===\n";
        
        std::cout << "Enter patient ID: ";
        uint32_t patient_id;
        std::cin >> patient_id;
        
        std::cout << "Enter doctor ID: ";
        uint32_t doctor_id;
        std::cin >> doctor_id;
        std::cin.ignore();
        
        std::cout << "Enter diagnosis: ";
        std::string diagnosis;
        std::getline(std::cin, diagnosis);
        
        std::cout << "Enter treatment: ";
        std::string treatment;
        std::getline(std::cin, treatment);
        
        std::cout << "Enter medication: ";
        std::string medication;
        std::getline(std::cin, medication);
        
        std::cout << "Follow-up required? (y/n): ";
        char follow_up_char;
        std::cin >> follow_up_char;
        bool follow_up = (follow_up_char == 'y' || follow_up_char == 'Y');
        
        TPacketMedicalRecord packet;
        packet.bHeader = HEADER_MEDICAL_RECORD;
        packet.patient_id = patient_id;
        packet.doctor_id = doctor_id;
        strncpy(packet.diagnosis, diagnosis.c_str(), sizeof(packet.diagnosis) - 1);
        packet.diagnosis[sizeof(packet.diagnosis) - 1] = '\0';
        strncpy(packet.treatment, treatment.c_str(), sizeof(packet.treatment) - 1);
        packet.treatment[sizeof(packet.treatment) - 1] = '\0';
        strncpy(packet.medication, medication.c_str(), sizeof(packet.medication) - 1);
        packet.medication[sizeof(packet.medication) - 1] = '\0';
        packet.visit_date = static_cast<uint32_t>(time(nullptr));
        packet.follow_up_required = follow_up;
        
        // Send header first
        uint8_t header = HEADER_MEDICAL_RECORD;
        send(sock_fd, &header, sizeof(header), 0);
        
        // Send packet
        send(sock_fd, &packet, sizeof(packet), 0);
        
        // Receive response
        TPacketResponse response;
        recv(sock_fd, &response, sizeof(response), 0);
        
        std::cout << "Server response: " << response.message << std::endl;
    }

    void show_menu() {
        std::cout << "\n=== Hospital Management System ===\n";
        std::cout << "1. Register Patient\n";
        std::cout << "2. Search Patient\n";
        std::cout << "3. Book Appointment\n";
        std::cout << "4. Add Medical Record\n";
        std::cout << "5. Exit\n";
        std::cout << "Enter your choice: ";
    }

    void run() {
        if (!connect_to_server()) {
            return;
        }

        int choice;
        do {
            show_menu();
            std::cin >> choice;
            std::cin.ignore();

            switch (choice) {
                case 1:
                    register_patient();
                    break;
                case 2:
                    search_patient();
                    break;
                case 3:
                    book_appointment();
                    break;
                case 4:
                    add_medical_record();
                    break;
                case 5:
                    std::cout << "Goodbye!\n";
                    break;
                default:
                    std::cout << "Invalid choice. Please try again.\n";
                    break;
            }
        } while (choice != 5);
    }
};

int main() {
    HospitalClient client;
    client.run();
    return 0;
}