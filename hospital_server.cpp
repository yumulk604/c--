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
#include <string>
#include <algorithm>

// Simple in-memory database structures
struct Patient {
    uint32_t id;
    std::string name;
    std::string surname;
    std::string phone;
    std::string address;
    uint8_t age;
    char gender;
    std::string blood_type;
    uint32_t registration_date;
};

struct Doctor {
    uint32_t id;
    std::string name;
    std::string specialization;
    std::map<uint8_t, std::pair<uint8_t, uint8_t>> schedule; // day -> (start_hour, end_hour)
};

struct Appointment {
    uint32_t patient_id;
    uint32_t doctor_id;
    uint8_t day_of_week;
    uint8_t hour;
    std::string reason;
    uint32_t appointment_date;
};

struct MedicalRecord {
    uint32_t patient_id;
    uint32_t doctor_id;
    std::string diagnosis;
    std::string treatment;
    std::string medication;
    uint32_t visit_date;
    bool follow_up_required;
};

class HospitalDatabase {
private:
    std::vector<Patient> patients;
    std::vector<Doctor> doctors;
    std::vector<Appointment> appointments;
    std::vector<MedicalRecord> medical_records;
    uint32_t next_patient_id;
    uint32_t next_doctor_id;

public:
    HospitalDatabase() : next_patient_id(1), next_doctor_id(1) {
        // Initialize with some sample doctors
        add_doctor("Dr. Smith", "Cardiology");
        add_doctor("Dr. Johnson", "Neurology");
        add_doctor("Dr. Williams", "Pediatrics");
        add_doctor("Dr. Brown", "Orthopedics");
        
        // Set up doctor schedules (Monday to Friday, 9 AM to 5 PM)
        for (auto& doctor : doctors) {
            for (int day = 0; day < 5; day++) {
                doctor.schedule[day] = {9, 17};
            }
        }
    }

    uint32_t add_patient(const TPacketPatientRegister& packet) {
        Patient patient;
        patient.id = next_patient_id++;
        patient.name = packet.name;
        patient.surname = packet.surname;
        patient.phone = packet.phone;
        patient.address = packet.address;
        patient.age = packet.age;
        patient.gender = packet.gender;
        patient.blood_type = packet.blood_type;
        patient.registration_date = static_cast<uint32_t>(time(nullptr));
        
        patients.push_back(patient);
        return patient.id;
    }

    void add_doctor(const std::string& name, const std::string& specialization) {
        Doctor doctor;
        doctor.id = next_doctor_id++;
        doctor.name = name;
        doctor.specialization = specialization;
        doctors.push_back(doctor);
    }

    Patient* find_patient_by_name(const std::string& name) {
        for (auto& patient : patients) {
            if (patient.name == name || patient.surname == name) {
                return &patient;
            }
        }
        return nullptr;
    }

    Patient* find_patient_by_phone(const std::string& phone) {
        for (auto& patient : patients) {
            if (patient.phone == phone) {
                return &patient;
            }
        }
        return nullptr;
    }

    Patient* find_patient_by_id(uint32_t id) {
        for (auto& patient : patients) {
            if (patient.id == id) {
                return &patient;
            }
        }
        return nullptr;
    }

    Doctor* find_doctor_by_id(uint32_t id) {
        for (auto& doctor : doctors) {
            if (doctor.id == id) {
                return &doctor;
            }
        }
        return nullptr;
    }

    bool book_appointment(uint32_t patient_id, uint32_t doctor_id, uint8_t day, uint8_t hour, const std::string& reason) {
        // Check if doctor is available
        Doctor* doctor = find_doctor_by_id(doctor_id);
        if (!doctor) return false;
        
        auto schedule_it = doctor->schedule.find(day);
        if (schedule_it == doctor->schedule.end()) return false;
        
        if (hour < schedule_it->second.first || hour >= schedule_it->second.second) {
            return false;
        }
        
        // Check for conflicts
        for (const auto& appointment : appointments) {
            if (appointment.doctor_id == doctor_id && 
                appointment.day_of_week == day && 
                appointment.hour == hour) {
                return false;
            }
        }
        
        Appointment appointment;
        appointment.patient_id = patient_id;
        appointment.doctor_id = doctor_id;
        appointment.day_of_week = day;
        appointment.hour = hour;
        appointment.reason = reason;
        appointment.appointment_date = static_cast<uint32_t>(time(nullptr));
        
        appointments.push_back(appointment);
        return true;
    }

    void add_medical_record(uint32_t patient_id, uint32_t doctor_id, 
                           const std::string& diagnosis, const std::string& treatment,
                           const std::string& medication, bool follow_up) {
        MedicalRecord record;
        record.patient_id = patient_id;
        record.doctor_id = doctor_id;
        record.diagnosis = diagnosis;
        record.treatment = treatment;
        record.medication = medication;
        record.visit_date = static_cast<uint32_t>(time(nullptr));
        record.follow_up_required = follow_up;
        
        medical_records.push_back(record);
    }

    std::vector<Doctor> get_doctors() const { return doctors; }
    std::vector<Patient> get_patients() const { return patients; }
    std::vector<Appointment> get_appointments() const { return appointments; }
    std::vector<MedicalRecord> get_medical_records() const { return medical_records; }
};

HospitalDatabase hospital_db;

void send_response(int client_fd, uint8_t result, const char* message) {
    TPacketResponse response;
    init_response(response, result, message);
    send(client_fd, &response, sizeof(response), 0);
}

void handle_patient_register(int client_fd, const TPacketPatientRegister& packet) {
    uint32_t patient_id = hospital_db.add_patient(packet);
    char response_msg[128];
    snprintf(response_msg, sizeof(response_msg), "Patient registered successfully with ID: %u", patient_id);
    send_response(client_fd, 0, response_msg);
}

void handle_patient_search(int client_fd, const TPacketPatientSearch& packet) {
    Patient* patient = nullptr;
    
    switch (packet.search_type) {
        case 0: // Search by name
            patient = hospital_db.find_patient_by_name(packet.search_term);
            break;
        case 1: // Search by phone
            patient = hospital_db.find_patient_by_phone(packet.search_term);
            break;
        case 2: // Search by ID
            {
                uint32_t id = static_cast<uint32_t>(std::stoul(packet.search_term));
                patient = hospital_db.find_patient_by_id(id);
            }
            break;
    }
    
    if (patient) {
        TPacketPatientInfo info;
        info.bHeader = HEADER_PATIENT_INFO;
        info.patient_id = patient->id;
        strncpy(info.name, patient->name.c_str(), sizeof(info.name) - 1);
        info.name[sizeof(info.name) - 1] = '\0';
        strncpy(info.surname, patient->surname.c_str(), sizeof(info.surname) - 1);
        info.surname[sizeof(info.surname) - 1] = '\0';
        strncpy(info.phone, patient->phone.c_str(), sizeof(info.phone) - 1);
        info.phone[sizeof(info.phone) - 1] = '\0';
        strncpy(info.address, patient->address.c_str(), sizeof(info.address) - 1);
        info.address[sizeof(info.address) - 1] = '\0';
        info.age = patient->age;
        info.gender = patient->gender;
        strncpy(info.blood_type, patient->blood_type.c_str(), sizeof(info.blood_type) - 1);
        info.blood_type[sizeof(info.blood_type) - 1] = '\0';
        info.registration_date = patient->registration_date;
        
        send(client_fd, &info, sizeof(info), 0);
    } else {
        send_response(client_fd, 1, "Patient not found");
    }
}

void handle_appointment_booking(int client_fd, const TPacketAppointmentBook& packet) {
    bool success = hospital_db.book_appointment(packet.patient_id, packet.doctor_id, 
                                               packet.day_of_week, packet.hour, packet.reason);
    
    if (success) {
        send_response(client_fd, 0, "Appointment booked successfully");
    } else {
        send_response(client_fd, 1, "Failed to book appointment - doctor not available or time slot taken");
    }
}

void handle_medical_record(int client_fd, const TPacketMedicalRecord& packet) {
    hospital_db.add_medical_record(packet.patient_id, packet.doctor_id,
                                  packet.diagnosis, packet.treatment,
                                  packet.medication, packet.follow_up_required);
    send_response(client_fd, 0, "Medical record added successfully");
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

    std::cout << "Hospital Management Server started on port 13000\n";
    std::cout << "Available operations:\n";
    std::cout << "- Patient Registration (0x01)\n";
    std::cout << "- Patient Search (0x02)\n";
    std::cout << "- Appointment Booking (0x05)\n";
    std::cout << "- Medical Record Entry (0x06)\n";

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