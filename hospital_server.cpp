#include "hospital_packet.h"
#include "hospital_db.h"
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
#include <string>
#include <algorithm>

// Global TimescaleDB database instance
HospitalDatabase hospital_db;

void send_response(int client_fd, uint8_t result, const char* message) {
    TPacketResponse response;
    init_response(response, result, message);
    send(client_fd, &response, sizeof(response), 0);
}

void handle_patient_register(int client_fd, const TPacketPatientRegister& packet) {
    uint32_t patient_id = hospital_db.add_patient(packet.name, packet.surname, packet.phone, 
                                                 packet.address, packet.age, packet.gender, 
                                                 packet.blood_type);
    if (patient_id > 0) {
        char response_msg[128];
        snprintf(response_msg, sizeof(response_msg), "Patient registered successfully with ID: %u", patient_id);
        send_response(client_fd, 0, response_msg);
    } else {
        send_response(client_fd, 1, "Failed to register patient - database error");
    }
}

void handle_patient_search(int client_fd, const TPacketPatientSearch& packet) {
    std::string name, surname, phone, address, blood_type;
    uint8_t age;
    char gender;
    uint32_t id, registration_date;
    bool found = false;
    
    switch (packet.search_type) {
        case 0: // Search by name
            found = hospital_db.find_patient_by_name(packet.search_term, id, surname, phone,
                                                   address, age, gender, blood_type, registration_date);
            if (found) name = packet.search_term;
            break;
        case 1: // Search by phone
            found = hospital_db.find_patient_by_phone(packet.search_term, id, name, surname,
                                                   address, age, gender, blood_type, registration_date);
            if (found) phone = packet.search_term;
            break;
        case 2: // Search by ID
            {
                uint32_t search_id = static_cast<uint32_t>(std::stoul(packet.search_term));
                found = hospital_db.find_patient_by_id(search_id, name, surname, phone,
                                                     address, age, gender, blood_type, registration_date);
                if (found) id = search_id;
            }
            break;
    }
    
    if (found) {
        TPacketPatientInfo info;
        info.bHeader = HEADER_PATIENT_INFO;
        info.patient_id = id;
        strncpy(info.name, name.c_str(), sizeof(info.name) - 1);
        info.name[sizeof(info.name) - 1] = '\0';
        strncpy(info.surname, surname.c_str(), sizeof(info.surname) - 1);
        info.surname[sizeof(info.surname) - 1] = '\0';
        strncpy(info.phone, phone.c_str(), sizeof(info.phone) - 1);
        info.phone[sizeof(info.phone) - 1] = '\0';
        strncpy(info.address, address.c_str(), sizeof(info.address) - 1);
        info.address[sizeof(info.address) - 1] = '\0';
        info.age = age;
        info.gender = gender;
        strncpy(info.blood_type, blood_type.c_str(), sizeof(info.blood_type) - 1);
        info.blood_type[sizeof(info.blood_type) - 1] = '\0';
        info.registration_date = registration_date;
        
        send(client_fd, &info, sizeof(info), 0);
    } else {
        send_response(client_fd, 1, "Patient not found");
    }
}

void handle_appointment_booking(int client_fd, const TPacketAppointmentBook& packet) {
    // Check if appointment slot is available
    if (!hospital_db.is_appointment_available(packet.doctor_id, packet.day_of_week, packet.hour)) {
        send_response(client_fd, 1, "Appointment slot not available");
        return;
    }
    
    bool success = hospital_db.book_appointment(packet.patient_id, packet.doctor_id, 
                                               packet.day_of_week, packet.hour, packet.reason);
    
    if (success) {
        send_response(client_fd, 0, "Appointment booked successfully");
    } else {
        send_response(client_fd, 1, "Failed to book appointment - database error");
    }
}

void handle_medical_record(int client_fd, const TPacketMedicalRecord& packet) {
    bool success = hospital_db.add_medical_record(packet.patient_id, packet.doctor_id,
                                                 packet.diagnosis, packet.treatment,
                                                 packet.medication, packet.follow_up_required);
    if (success) {
        send_response(client_fd, 0, "Medical record added successfully");
    } else {
        send_response(client_fd, 1, "Failed to add medical record - database error");
    }
}

void handle_client_request(int client_fd, uint8_t header) {
    switch (header) {
        case HEADER_PATIENT_REGISTER: {
            TPacketPatientRegister packet;
            ssize_t received = recv(client_fd, &packet, sizeof(packet), 0);
            if (received == sizeof(packet)) {
                handle_patient_register(client_fd, packet);
            } else {
                send_response(client_fd, 1, "Invalid patient registration packet");
            }
            break;
        }
        case HEADER_PATIENT_SEARCH: {
            TPacketPatientSearch packet;
            ssize_t received = recv(client_fd, &packet, sizeof(packet), 0);
            if (received == sizeof(packet)) {
                handle_patient_search(client_fd, packet);
            } else {
                send_response(client_fd, 1, "Invalid patient search packet");
            }
            break;
        }
        case HEADER_APPOINTMENT_BOOK: {
            TPacketAppointmentBook packet;
            ssize_t received = recv(client_fd, &packet, sizeof(packet), 0);
            if (received == sizeof(packet)) {
                handle_appointment_booking(client_fd, packet);
            } else {
                send_response(client_fd, 1, "Invalid appointment booking packet");
            }
            break;
        }
        case HEADER_MEDICAL_RECORD: {
            TPacketMedicalRecord packet;
            ssize_t received = recv(client_fd, &packet, sizeof(packet), 0);
            if (received == sizeof(packet)) {
                handle_medical_record(client_fd, packet);
            } else {
                send_response(client_fd, 1, "Invalid medical record packet");
            }
            break;
        }
        default:
            send_response(client_fd, 1, "Unknown packet type");
            break;
    }
}

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));

    // Initialize TimescaleDB connection
    std::cout << "Connecting to TimescaleDB..." << std::endl;
    if (!hospital_db.connect()) {
        std::cerr << "Failed to connect to TimescaleDB. Please check your database configuration." << std::endl;
        return 1;
    }

    // Initialize database schema
    std::cout << "Initializing database schema..." << std::endl;
    if (!hospital_db.create_tables()) {
        std::cerr << "Failed to create database tables" << std::endl;
        return 1;
    }

    if (!hospital_db.create_hypertables()) {
        std::cerr << "Failed to create TimescaleDB hypertables" << std::endl;
        return 1;
    }

    if (!hospital_db.insert_sample_data()) {
        std::cerr << "Failed to insert sample data" << std::endl;
        return 1;
    }

    std::cout << "Database initialized successfully!" << std::endl;
    std::cout << "Patients: " << hospital_db.get_patient_count() << std::endl;
    std::cout << "Appointments: " << hospital_db.get_appointment_count() << std::endl;
    std::cout << "Medical Records: " << hospital_db.get_medical_record_count() << std::endl;

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

    std::cout << "\nHospital Management Server started on port 13000" << std::endl;
    std::cout << "Available operations:" << std::endl;
    std::cout << "- Patient Registration (0x01)" << std::endl;
    std::cout << "- Patient Search (0x02)" << std::endl;
    std::cout << "- Appointment Booking (0x05)" << std::endl;
    std::cout << "- Medical Record Entry (0x06)" << std::endl;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        std::cout << "Client connected: " << inet_ntoa(client_addr.sin_addr) << std::endl;

        // Read packet header
        uint8_t header;
        ssize_t received = recv(client_fd, &header, sizeof(header), 0);
        if (received != sizeof(header)) {
            std::cerr << "Failed to receive packet header" << std::endl;
            close(client_fd);
            continue;
        }

        std::cout << "Received packet header: 0x" << std::hex << static_cast<int>(header) << std::dec << std::endl;

        // Handle the request
        handle_client_request(client_fd, header);

        close(client_fd);
    }

    close(server_fd);
    return 0;
}