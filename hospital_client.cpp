#include "packet.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <ctime>

uint32_t get_dword_time() {
    return static_cast<uint32_t>(time(nullptr));
}

int main() {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(13000);

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        return 1;
    }

    if (connect(client_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        perror("connect");
        return 1;
    }

    std::cout << "Connected to Hospital Management Server" << std::endl;

    // Handshake
    TPacketCGHandshake handshake;
    handshake.bHeader = HEADER_CG_HANDSHAKE;
    handshake.dwHandshake = 0;
    handshake.dwTime = get_dword_time();
    handshake.lDelta = 0;

    if (send(client_fd, &handshake, sizeof(handshake), 0) != sizeof(handshake)) {
        std::cerr << "Failed to send handshake" << std::endl;
        return 1;
    }

    TPacketGCHandshake response;
    if (recv(client_fd, &response, sizeof(response), 0) != sizeof(response)) {
        std::cerr << "Failed to receive handshake response" << std::endl;
        return 1;
    }

    handshake.dwHandshake = response.dwHandshake;
    if (send(client_fd, &handshake, sizeof(handshake), 0) != sizeof(handshake)) {
        std::cerr << "Failed to confirm handshake" << std::endl;
        return 1;
    }

    std::cout << "Handshake successful!" << std::endl;

    // Main menu
    while (true) {
        std::cout << "\n=== Hospital Management System ===" << std::endl;
        std::cout << "1. Login" << std::endl;
        std::cout << "2. Register Patient" << std::endl;
        std::cout << "3. Search Patient" << std::endl;
        std::cout << "4. Register Doctor" << std::endl;
        std::cout << "5. List Doctors" << std::endl;
        std::cout << "6. Book Appointment" << std::endl;
        std::cout << "7. Add Medical Record" << std::endl;
        std::cout << "8. Logout" << std::endl;
        std::cout << "9. Exit" << std::endl;
        std::cout << "Choose option: ";

        int choice;
        std::cin >> choice;

        if (choice == 9) {
            break;
        }

        switch (choice) {
            case 1: {  // Login
                TPacketCSLogin loginPacket;
                loginPacket.bHeader = HEADER_CS_LOGIN;

                std::cout << "Username: ";
                std::cin >> loginPacket.szUsername;
                std::cout << "Password: ";
                std::cin >> loginPacket.szPassword;

                send(client_fd, &loginPacket, sizeof(loginPacket), 0);

                TPacketSCLoginResponse loginResponse;
                if (recv(client_fd, &loginResponse, sizeof(loginResponse), 0) == sizeof(loginResponse)) {
                    if (loginResponse.bSuccess) {
                        std::cout << "Login successful! Welcome " << loginResponse.szUserType << std::endl;
                    } else {
                        std::cout << "Login failed!" << std::endl;
                    }
                }
                break;
            }

            case 2: {  // Register Patient
                TPacketCSPatientRegister patientPacket;
                patientPacket.bHeader = HEADER_CS_PATIENT_REGISTER;

                std::cout << "Patient Name: ";
                std::cin >> patientPacket.szName;
                std::cout << "Surname: ";
                std::cin >> patientPacket.szSurname;
                std::cout << "TC Number: ";
                std::cin >> patientPacket.szTCNumber;
                std::cout << "Birth Date (YYYY-MM-DD): ";
                std::cin >> patientPacket.szBirthDate;
                std::cout << "Phone: ";
                std::cin >> patientPacket.szPhone;
                std::cout << "Address: ";
                std::cin.ignore();
                std::cin.getline(patientPacket.szAddress, sizeof(patientPacket.szAddress));
                std::cout << "Gender (M/F): ";
                std::cin >> patientPacket.szGender;

                send(client_fd, &patientPacket, sizeof(patientPacket), 0);

                // Wait for response
                char buffer[256];
                recv(client_fd, buffer, sizeof(buffer), 0);
                std::cout << "Server response: " << buffer << std::endl;
                break;
            }

            case 3: {  // Search Patient
                TPacketCSPatientSearch searchPacket;
                searchPacket.bHeader = HEADER_CS_PATIENT_SEARCH;

                std::cout << "Search term (name, surname, or TC): ";
                std::cin >> searchPacket.szSearchTerm;

                send(client_fd, &searchPacket, sizeof(searchPacket), 0);

                TPacketSCPatientList patientList;
                if (recv(client_fd, &patientList, sizeof(patientList), 0) == sizeof(patientList)) {
                    std::cout << "Found " << patientList.dwCount << " patients:" << std::endl;
                    for (uint32_t i = 0; i < patientList.dwCount; ++i) {
                        TPacketSCPatientData patient;
                        if (recv(client_fd, &patient, sizeof(patient), 0) == sizeof(patient)) {
                            std::cout << "ID: " << patient.dwPatientID
                                      << ", Name: " << patient.szName << " " << patient.szSurname
                                      << ", TC: " << patient.szTCNumber << std::endl;
                        }
                    }
                }
                break;
            }

            case 4: {  // Register Doctor
                TPacketCSRegisterDoctor doctorPacket;
                doctorPacket.bHeader = HEADER_CS_DOCTOR_REGISTER;

                std::cout << "Doctor Name: ";
                std::cin >> doctorPacket.szName;
                std::cout << "Surname: ";
                std::cin >> doctorPacket.szSurname;
                std::cout << "Specialty: ";
                std::cin >> doctorPacket.szSpecialty;
                std::cout << "Phone: ";
                std::cin >> doctorPacket.szPhone;
                std::cout << "Email: ";
                std::cin >> doctorPacket.szEmail;

                send(client_fd, &doctorPacket, sizeof(doctorPacket), 0);

                char buffer[256];
                recv(client_fd, buffer, sizeof(buffer), 0);
                std::cout << "Server response: " << buffer << std::endl;
                break;
            }

            case 5: {  // List Doctors
                uint8_t header = HEADER_CS_DOCTOR_LIST;
                send(client_fd, &header, sizeof(header), 0);

                uint32_t doctorCount;
                if (recv(client_fd, &doctorCount, sizeof(doctorCount), 0) == sizeof(doctorCount)) {
                    std::cout << "Available Doctors (" << doctorCount << "):" << std::endl;
                    for (uint32_t i = 0; i < doctorCount; ++i) {
                        uint32_t length;
                        if (recv(client_fd, &length, sizeof(length), 0) == sizeof(length)) {
                            char* doctorInfo = new char[length + 1];
                            if (recv(client_fd, doctorInfo, length, 0) == length) {
                                doctorInfo[length] = '\0';
                                std::cout << "- " << doctorInfo << std::endl;
                            }
                            delete[] doctorInfo;
                        }
                    }
                }
                break;
            }

            case 6: {  // Book Appointment
                TPacketCSBookAppointment appointmentPacket;
                appointmentPacket.bHeader = HEADER_CS_APPOINTMENT_BOOK;

                std::cout << "Patient ID: ";
                std::cin >> appointmentPacket.dwPatientID;
                std::cout << "Doctor ID: ";
                std::cin >> appointmentPacket.dwDoctorID;
                std::cout << "Appointment Date (YYYY-MM-DD HH:MM): ";
                std::cin >> appointmentPacket.szAppointmentDate;
                std::cout << "Notes: ";
                std::cin.ignore();
                std::cin.getline(appointmentPacket.szNotes, sizeof(appointmentPacket.szNotes));

                send(client_fd, &appointmentPacket, sizeof(appointmentPacket), 0);

                TPacketSCAppointmentConfirm confirm;
                if (recv(client_fd, &confirm, sizeof(confirm), 0) == sizeof(confirm)) {
                    std::cout << "Appointment confirmed!" << std::endl;
                    std::cout << "Appointment ID: " << confirm.dwAppointmentID << std::endl;
                    std::cout << "Date: " << confirm.szAppointmentDate << std::endl;
                    std::cout << "Status: " << confirm.szStatus << std::endl;
                }
                break;
            }

            case 7: {  // Add Medical Record
                TPacketCSAddMedicalRecord recordPacket;
                recordPacket.bHeader = HEADER_CS_MEDICAL_RECORD_ADD;

                std::cout << "Patient ID: ";
                std::cin >> recordPacket.dwPatientID;
                std::cout << "Doctor ID: ";
                std::cin >> recordPacket.dwDoctorID;
                std::cout << "Diagnosis: ";
                std::cin.ignore();
                std::cin.getline(recordPacket.szDiagnosis, sizeof(recordPacket.szDiagnosis));
                std::cout << "Treatment: ";
                std::cin.getline(recordPacket.szTreatment, sizeof(recordPacket.szTreatment));
                std::cout << "Medications: ";
                std::cin.getline(recordPacket.szMedications, sizeof(recordPacket.szMedications));
                std::cout << "Notes: ";
                std::cin.getline(recordPacket.szNotes, sizeof(recordPacket.szNotes));

                send(client_fd, &recordPacket, sizeof(recordPacket), 0);

                char buffer[256];
                recv(client_fd, buffer, sizeof(buffer), 0);
                std::cout << "Server response: " << buffer << std::endl;
                break;
            }

            case 8: {  // Logout
                uint8_t header = HEADER_CS_LOGOUT;
                send(client_fd, &header, sizeof(header), 0);
                std::cout << "Logged out successfully!" << std::endl;
                break;
            }

            default:
                std::cout << "Invalid option!" << std::endl;
                break;
        }
    }

    close(client_fd);
    return 0;
}