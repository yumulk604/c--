#include "hospital_db.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <vector>
#include <string>

class HospitalAdmin {
private:
    HospitalDatabase db;

public:
    HospitalAdmin() {
        if (!db.connect()) {
            std::cerr << "Failed to connect to TimescaleDB" << std::endl;
            exit(1);
        }
    }

    ~HospitalAdmin() {
        db.disconnect();
    }

    void show_main_menu() {
        std::cout << "\n=== Hospital Administration System ===" << std::endl;
        std::cout << "1. Database Statistics" << std::endl;
        std::cout << "2. Patient Management" << std::endl;
        std::cout << "3. Doctor Management" << std::endl;
        std::cout << "4. Appointment Management" << std::endl;
        std::cout << "5. Medical Records" << std::endl;
        std::cout << "6. Database Maintenance" << std::endl;
        std::cout << "7. Exit" << std::endl;
        std::cout << "Enter your choice: ";
    }

    void show_database_statistics() {
        std::cout << "\n=== Database Statistics ===" << std::endl;
        std::cout << "Total Patients: " << db.get_patient_count() << std::endl;
        std::cout << "Total Appointments: " << db.get_appointment_count() << std::endl;
        std::cout << "Total Medical Records: " << db.get_medical_record_count() << std::endl;
        
        auto doctors = db.get_all_doctors();
        std::cout << "Total Doctors: " << doctors.size() << std::endl;
        
        std::cout << "\nDoctors List:" << std::endl;
        for (const auto& doctor : doctors) {
            std::cout << "  ID " << doctor.first << ": " << doctor.second << std::endl;
        }
    }

    void show_patient_management() {
        std::cout << "\n=== Patient Management ===" << std::endl;
        std::cout << "1. Search Patient" << std::endl;
        std::cout << "2. Add New Patient" << std::endl;
        std::cout << "3. Back to Main Menu" << std::endl;
        std::cout << "Enter your choice: ";
        
        int choice;
        std::cin >> choice;
        std::cin.ignore();
        
        switch (choice) {
            case 1:
                search_patient();
                break;
            case 2:
                add_patient();
                break;
            case 3:
                return;
            default:
                std::cout << "Invalid choice" << std::endl;
        }
    }

    void search_patient() {
        std::cout << "\n=== Search Patient ===" << std::endl;
        std::cout << "Search by:" << std::endl;
        std::cout << "1. Name" << std::endl;
        std::cout << "2. Phone" << std::endl;
        std::cout << "3. ID" << std::endl;
        std::cout << "Enter choice: ";
        
        int search_type;
        std::cin >> search_type;
        std::cin.ignore();
        
        std::string search_term;
        std::cout << "Enter search term: ";
        std::getline(std::cin, search_term);
        
        std::string name, surname, phone, address, blood_type;
        uint8_t age;
        char gender;
        uint32_t id, registration_date;
        bool found = false;
        
        switch (search_type) {
            case 1:
                found = db.find_patient_by_name(search_term, id, surname, phone,
                                               address, age, gender, blood_type, registration_date);
                if (found) name = search_term;
                break;
            case 2:
                found = db.find_patient_by_phone(search_term, id, name, surname,
                                                address, age, gender, blood_type, registration_date);
                if (found) phone = search_term;
                break;
            case 3:
                {
                    uint32_t search_id = static_cast<uint32_t>(std::stoul(search_term));
                    found = db.find_patient_by_id(search_id, name, surname, phone,
                                                 address, age, gender, blood_type, registration_date);
                    if (found) id = search_id;
                }
                break;
        }
        
        if (found) {
            std::cout << "\n=== Patient Information ===" << std::endl;
            std::cout << "ID: " << id << std::endl;
            std::cout << "Name: " << name << " " << surname << std::endl;
            std::cout << "Phone: " << phone << std::endl;
            std::cout << "Address: " << address << std::endl;
            std::cout << "Age: " << static_cast<int>(age) << std::endl;
            std::cout << "Gender: " << gender << std::endl;
            std::cout << "Blood Type: " << blood_type << std::endl;
            
            time_t reg_time = static_cast<time_t>(registration_date);
            std::cout << "Registration Date: " << ctime(&reg_time);
        } else {
            std::cout << "Patient not found" << std::endl;
        }
    }

    void add_patient() {
        std::cout << "\n=== Add New Patient ===" << std::endl;
        
        std::string name, surname, phone, address, blood_type;
        int age;
        char gender;
        
        std::cout << "Enter name: ";
        std::getline(std::cin, name);
        
        std::cout << "Enter surname: ";
        std::getline(std::cin, surname);
        
        std::cout << "Enter phone: ";
        std::getline(std::cin, phone);
        
        std::cout << "Enter address: ";
        std::getline(std::cin, address);
        
        std::cout << "Enter age: ";
        std::cin >> age;
        std::cin.ignore();
        
        std::cout << "Enter gender (M/F): ";
        std::cin >> gender;
        std::cin.ignore();
        
        std::cout << "Enter blood type: ";
        std::getline(std::cin, blood_type);
        
        uint32_t patient_id = db.add_patient(name, surname, phone, address, 
                                            static_cast<uint8_t>(age), gender, blood_type);
        
        if (patient_id > 0) {
            std::cout << "Patient added successfully with ID: " << patient_id << std::endl;
        } else {
            std::cout << "Failed to add patient" << std::endl;
        }
    }

    void show_doctor_management() {
        std::cout << "\n=== Doctor Management ===" << std::endl;
        std::cout << "1. View All Doctors" << std::endl;
        std::cout << "2. Add New Doctor" << std::endl;
        std::cout << "3. Back to Main Menu" << std::endl;
        std::cout << "Enter your choice: ";
        
        int choice;
        std::cin >> choice;
        std::cin.ignore();
        
        switch (choice) {
            case 1:
                view_doctors();
                break;
            case 2:
                add_doctor();
                break;
            case 3:
                return;
            default:
                std::cout << "Invalid choice" << std::endl;
        }
    }

    void view_doctors() {
        std::cout << "\n=== All Doctors ===" << std::endl;
        auto doctors = db.get_all_doctors();
        
        if (doctors.empty()) {
            std::cout << "No doctors found" << std::endl;
            return;
        }
        
        std::cout << std::setw(5) << "ID" << std::setw(30) << "Name & Specialization" << std::endl;
        std::cout << std::string(35, '-') << std::endl;
        
        for (const auto& doctor : doctors) {
            std::cout << std::setw(5) << doctor.first << std::setw(30) << doctor.second << std::endl;
        }
    }

    void add_doctor() {
        std::cout << "\n=== Add New Doctor ===" << std::endl;
        
        std::string name, specialization;
        
        std::cout << "Enter doctor name: ";
        std::getline(std::cin, name);
        
        std::cout << "Enter specialization: ";
        std::getline(std::cin, specialization);
        
        if (db.add_doctor(name, specialization)) {
            std::cout << "Doctor added successfully" << std::endl;
        } else {
            std::cout << "Failed to add doctor" << std::endl;
        }
    }

    void show_appointment_management() {
        std::cout << "\n=== Appointment Management ===" << std::endl;
        std::cout << "1. Book Appointment" << std::endl;
        std::cout << "2. Check Availability" << std::endl;
        std::cout << "3. Back to Main Menu" << std::endl;
        std::cout << "Enter your choice: ";
        
        int choice;
        std::cin >> choice;
        std::cin.ignore();
        
        switch (choice) {
            case 1:
                book_appointment();
                break;
            case 2:
                check_availability();
                break;
            case 3:
                return;
            default:
                std::cout << "Invalid choice" << std::endl;
        }
    }

    void book_appointment() {
        std::cout << "\n=== Book Appointment ===" << std::endl;
        
        uint32_t patient_id, doctor_id;
        int day, hour;
        std::string reason;
        
        std::cout << "Enter patient ID: ";
        std::cin >> patient_id;
        
        std::cout << "Enter doctor ID: ";
        std::cin >> doctor_id;
        
        std::cout << "Enter day of week (0=Monday, 6=Sunday): ";
        std::cin >> day;
        
        std::cout << "Enter hour (9-16): ";
        std::cin >> hour;
        std::cin.ignore();
        
        std::cout << "Enter reason: ";
        std::getline(std::cin, reason);
        
        if (db.book_appointment(patient_id, doctor_id, static_cast<uint8_t>(day), 
                               static_cast<uint8_t>(hour), reason)) {
            std::cout << "Appointment booked successfully" << std::endl;
        } else {
            std::cout << "Failed to book appointment" << std::endl;
        }
    }

    void check_availability() {
        std::cout << "\n=== Check Availability ===" << std::endl;
        
        uint32_t doctor_id;
        int day, hour;
        
        std::cout << "Enter doctor ID: ";
        std::cin >> doctor_id;
        
        std::cout << "Enter day of week (0=Monday, 6=Sunday): ";
        std::cin >> day;
        
        std::cout << "Enter hour (9-16): ";
        std::cin >> hour;
        
        if (db.is_appointment_available(doctor_id, static_cast<uint8_t>(day), static_cast<uint8_t>(hour))) {
            std::cout << "Time slot is available" << std::endl;
        } else {
            std::cout << "Time slot is not available" << std::endl;
        }
    }

    void show_medical_records() {
        std::cout << "\n=== Medical Records ===" << std::endl;
        std::cout << "1. Add Medical Record" << std::endl;
        std::cout << "2. View Patient Records" << std::endl;
        std::cout << "3. Back to Main Menu" << std::endl;
        std::cout << "Enter your choice: ";
        
        int choice;
        std::cin >> choice;
        std::cin.ignore();
        
        switch (choice) {
            case 1:
                add_medical_record();
                break;
            case 2:
                view_patient_records();
                break;
            case 3:
                return;
            default:
                std::cout << "Invalid choice" << std::endl;
        }
    }

    void add_medical_record() {
        std::cout << "\n=== Add Medical Record ===" << std::endl;
        
        uint32_t patient_id, doctor_id;
        std::string diagnosis, treatment, medication;
        char follow_up_char;
        
        std::cout << "Enter patient ID: ";
        std::cin >> patient_id;
        
        std::cout << "Enter doctor ID: ";
        std::cin >> doctor_id;
        std::cin.ignore();
        
        std::cout << "Enter diagnosis: ";
        std::getline(std::cin, diagnosis);
        
        std::cout << "Enter treatment: ";
        std::getline(std::cin, treatment);
        
        std::cout << "Enter medication: ";
        std::getline(std::cin, medication);
        
        std::cout << "Follow-up required? (y/n): ";
        std::cin >> follow_up_char;
        
        bool follow_up = (follow_up_char == 'y' || follow_up_char == 'Y');
        
        if (db.add_medical_record(patient_id, doctor_id, diagnosis, treatment, medication, follow_up)) {
            std::cout << "Medical record added successfully" << std::endl;
        } else {
            std::cout << "Failed to add medical record" << std::endl;
        }
    }

    void view_patient_records() {
        std::cout << "\n=== View Patient Records ===" << std::endl;
        
        uint32_t patient_id;
        std::cout << "Enter patient ID: ";
        std::cin >> patient_id;
        
        auto records = db.get_medical_records_by_patient(patient_id);
        
        if (records.empty()) {
            std::cout << "No medical records found for this patient" << std::endl;
            return;
        }
        
        std::cout << "\n=== Medical Records for Patient " << patient_id << " ===" << std::endl;
        
        for (const auto& record : records) {
            std::cout << "Doctor ID: " << std::get<1>(record) << std::endl;
            std::cout << "Diagnosis: " << std::get<2>(record) << std::endl;
            std::cout << "Treatment: " << std::get<3>(record) << std::endl;
            std::cout << "Medication: " << std::get<4>(record) << std::endl;
            
            time_t visit_time = static_cast<time_t>(std::get<5>(record));
            std::cout << "Visit Date: " << ctime(&visit_time);
            
            std::cout << "Follow-up Required: " << (std::get<6>(record) ? "Yes" : "No") << std::endl;
            std::cout << std::string(50, '-') << std::endl;
        }
    }

    void show_database_maintenance() {
        std::cout << "\n=== Database Maintenance ===" << std::endl;
        std::cout << "1. Recreate Tables" << std::endl;
        std::cout << "2. Create Hypertables" << std::endl;
        std::cout << "3. Insert Sample Data" << std::endl;
        std::cout << "4. Back to Main Menu" << std::endl;
        std::cout << "Enter your choice: ";
        
        int choice;
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                if (db.create_tables()) {
                    std::cout << "Tables created successfully" << std::endl;
                } else {
                    std::cout << "Failed to create tables" << std::endl;
                }
                break;
            case 2:
                if (db.create_hypertables()) {
                    std::cout << "Hypertables created successfully" << std::endl;
                } else {
                    std::cout << "Failed to create hypertables" << std::endl;
                }
                break;
            case 3:
                if (db.insert_sample_data()) {
                    std::cout << "Sample data inserted successfully" << std::endl;
                } else {
                    std::cout << "Failed to insert sample data" << std::endl;
                }
                break;
            case 4:
                return;
            default:
                std::cout << "Invalid choice" << std::endl;
        }
    }

    void run() {
        int choice;
        
        do {
            show_main_menu();
            std::cin >> choice;
            std::cin.ignore();
            
            switch (choice) {
                case 1:
                    show_database_statistics();
                    break;
                case 2:
                    show_patient_management();
                    break;
                case 3:
                    show_doctor_management();
                    break;
                case 4:
                    show_appointment_management();
                    break;
                case 5:
                    show_medical_records();
                    break;
                case 6:
                    show_database_maintenance();
                    break;
                case 7:
                    std::cout << "Goodbye!" << std::endl;
                    break;
                default:
                    std::cout << "Invalid choice. Please try again." << std::endl;
                    break;
            }
        } while (choice != 7);
    }
};

int main() {
    HospitalAdmin admin;
    admin.run();
    return 0;
}