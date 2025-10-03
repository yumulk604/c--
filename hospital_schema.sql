-- Hospital Management System Database Schema
-- Compatible with MySQL/MariaDB

CREATE DATABASE IF NOT EXISTS hospital_management;
USE hospital_management;

-- Users table (for authentication)
CREATE TABLE IF NOT EXISTS users (
    user_id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(32) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    user_type TINYINT NOT NULL, -- 1=Patient, 2=Doctor, 3=Nurse, 4=Admin
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    is_active BOOLEAN DEFAULT TRUE
);

-- Patients table
CREATE TABLE IF NOT EXISTS patients (
    patient_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT,
    first_name VARCHAR(32) NOT NULL,
    last_name VARCHAR(32) NOT NULL,
    date_of_birth DATE NOT NULL,
    gender ENUM('M', 'F', 'O') NOT NULL,
    phone VARCHAR(16),
    email VARCHAR(64),
    address TEXT,
    emergency_contact VARCHAR(32),
    emergency_phone VARCHAR(16),
    registration_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE SET NULL,
    INDEX idx_patient_name (last_name, first_name),
    INDEX idx_patient_phone (phone),
    INDEX idx_patient_email (email)
);

-- Departments table
CREATE TABLE IF NOT EXISTS departments (
    department_id INT AUTO_INCREMENT PRIMARY KEY,
    department_name VARCHAR(64) NOT NULL UNIQUE,
    description TEXT,
    head_doctor_id INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Doctors table
CREATE TABLE IF NOT EXISTS doctors (
    doctor_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT,
    first_name VARCHAR(32) NOT NULL,
    last_name VARCHAR(32) NOT NULL,
    specialization VARCHAR(32) NOT NULL,
    department_id INT,
    phone VARCHAR(16),
    email VARCHAR(64),
    office_room VARCHAR(16),
    license_number VARCHAR(32) UNIQUE,
    years_experience INT DEFAULT 0,
    is_available BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE SET NULL,
    FOREIGN KEY (department_id) REFERENCES departments(department_id) ON DELETE SET NULL,
    INDEX idx_doctor_specialization (specialization),
    INDEX idx_doctor_name (last_name, first_name),
    INDEX idx_doctor_available (is_available)
);

-- Add foreign key constraint for department head
ALTER TABLE departments 
ADD FOREIGN KEY (head_doctor_id) REFERENCES doctors(doctor_id) ON DELETE SET NULL;

-- Nurses table
CREATE TABLE IF NOT EXISTS nurses (
    nurse_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT,
    first_name VARCHAR(32) NOT NULL,
    last_name VARCHAR(32) NOT NULL,
    department_id INT,
    phone VARCHAR(16),
    email VARCHAR(64),
    shift_type ENUM('DAY', 'NIGHT', 'ROTATING') DEFAULT 'DAY',
    license_number VARCHAR(32) UNIQUE,
    is_available BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE SET NULL,
    FOREIGN KEY (department_id) REFERENCES departments(department_id) ON DELETE SET NULL
);

-- Appointments table
CREATE TABLE IF NOT EXISTS appointments (
    appointment_id INT AUTO_INCREMENT PRIMARY KEY,
    patient_id INT NOT NULL,
    doctor_id INT NOT NULL,
    appointment_date DATE NOT NULL,
    appointment_time TIME NOT NULL,
    reason TEXT,
    status ENUM('SCHEDULED', 'COMPLETED', 'CANCELLED', 'NO_SHOW') DEFAULT 'SCHEDULED',
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (patient_id) REFERENCES patients(patient_id) ON DELETE CASCADE,
    FOREIGN KEY (doctor_id) REFERENCES doctors(doctor_id) ON DELETE CASCADE,
    UNIQUE KEY unique_appointment (doctor_id, appointment_date, appointment_time),
    INDEX idx_appointment_patient (patient_id),
    INDEX idx_appointment_doctor (doctor_id),
    INDEX idx_appointment_date (appointment_date),
    INDEX idx_appointment_status (status)
);

-- Medical records table
CREATE TABLE IF NOT EXISTS medical_records (
    record_id INT AUTO_INCREMENT PRIMARY KEY,
    patient_id INT NOT NULL,
    doctor_id INT NOT NULL,
    appointment_id INT,
    record_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    diagnosis TEXT,
    treatment TEXT,
    prescription TEXT,
    notes TEXT,
    vital_signs JSON, -- Store blood pressure, temperature, etc.
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (patient_id) REFERENCES patients(patient_id) ON DELETE CASCADE,
    FOREIGN KEY (doctor_id) REFERENCES doctors(doctor_id) ON DELETE CASCADE,
    FOREIGN KEY (appointment_id) REFERENCES appointments(appointment_id) ON DELETE SET NULL,
    INDEX idx_medical_record_patient (patient_id),
    INDEX idx_medical_record_doctor (doctor_id),
    INDEX idx_medical_record_date (record_date)
);

-- Medications table
CREATE TABLE IF NOT EXISTS medications (
    medication_id INT AUTO_INCREMENT PRIMARY KEY,
    medication_name VARCHAR(128) NOT NULL UNIQUE,
    generic_name VARCHAR(128),
    manufacturer VARCHAR(64),
    dosage_form VARCHAR(32), -- tablet, capsule, syrup, etc.
    strength VARCHAR(32),
    description TEXT,
    side_effects TEXT,
    contraindications TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Prescriptions table
CREATE TABLE IF NOT EXISTS prescriptions (
    prescription_id INT AUTO_INCREMENT PRIMARY KEY,
    record_id INT NOT NULL,
    medication_id INT NOT NULL,
    dosage VARCHAR(64) NOT NULL,
    frequency VARCHAR(64) NOT NULL, -- "twice daily", "every 8 hours", etc.
    duration VARCHAR(32), -- "7 days", "2 weeks", etc.
    instructions TEXT,
    quantity_prescribed INT,
    refills_allowed INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (record_id) REFERENCES medical_records(record_id) ON DELETE CASCADE,
    FOREIGN KEY (medication_id) REFERENCES medications(medication_id) ON DELETE CASCADE
);

-- Hospital rooms table
CREATE TABLE IF NOT EXISTS rooms (
    room_id INT AUTO_INCREMENT PRIMARY KEY,
    room_number VARCHAR(16) NOT NULL UNIQUE,
    room_type ENUM('GENERAL', 'PRIVATE', 'ICU', 'EMERGENCY', 'SURGERY') NOT NULL,
    department_id INT,
    bed_count INT DEFAULT 1,
    is_occupied BOOLEAN DEFAULT FALSE,
    equipment TEXT, -- JSON or text description of available equipment
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (department_id) REFERENCES departments(department_id) ON DELETE SET NULL
);

-- Patient admissions table
CREATE TABLE IF NOT EXISTS admissions (
    admission_id INT AUTO_INCREMENT PRIMARY KEY,
    patient_id INT NOT NULL,
    room_id INT NOT NULL,
    doctor_id INT NOT NULL,
    admission_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    discharge_date TIMESTAMP NULL,
    admission_reason TEXT,
    discharge_summary TEXT,
    status ENUM('ADMITTED', 'DISCHARGED', 'TRANSFERRED') DEFAULT 'ADMITTED',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (patient_id) REFERENCES patients(patient_id) ON DELETE CASCADE,
    FOREIGN KEY (room_id) REFERENCES rooms(room_id) ON DELETE CASCADE,
    FOREIGN KEY (doctor_id) REFERENCES doctors(doctor_id) ON DELETE CASCADE,
    INDEX idx_admission_patient (patient_id),
    INDEX idx_admission_room (room_id),
    INDEX idx_admission_status (status)
);

-- Billing table
CREATE TABLE IF NOT EXISTS billing (
    bill_id INT AUTO_INCREMENT PRIMARY KEY,
    patient_id INT NOT NULL,
    appointment_id INT,
    admission_id INT,
    bill_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    total_amount DECIMAL(10,2) NOT NULL DEFAULT 0.00,
    paid_amount DECIMAL(10,2) DEFAULT 0.00,
    payment_status ENUM('PENDING', 'PARTIAL', 'PAID', 'OVERDUE') DEFAULT 'PENDING',
    payment_method VARCHAR(32),
    insurance_claim_number VARCHAR(64),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (patient_id) REFERENCES patients(patient_id) ON DELETE CASCADE,
    FOREIGN KEY (appointment_id) REFERENCES appointments(appointment_id) ON DELETE SET NULL,
    FOREIGN KEY (admission_id) REFERENCES admissions(admission_id) ON DELETE SET NULL,
    INDEX idx_billing_patient (patient_id),
    INDEX idx_billing_status (payment_status),
    INDEX idx_billing_date (bill_date)
);

-- Billing items table
CREATE TABLE IF NOT EXISTS billing_items (
    item_id INT AUTO_INCREMENT PRIMARY KEY,
    bill_id INT NOT NULL,
    service_name VARCHAR(128) NOT NULL,
    service_code VARCHAR(32),
    quantity INT DEFAULT 1,
    unit_price DECIMAL(10,2) NOT NULL,
    total_price DECIMAL(10,2) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (bill_id) REFERENCES billing(bill_id) ON DELETE CASCADE
);

-- Insert sample data
-- Sample departments
INSERT INTO departments (department_name, description) VALUES
('Emergency Medicine', 'Emergency and urgent care services'),
('Internal Medicine', 'General internal medicine and primary care'),
('Cardiology', 'Heart and cardiovascular system care'),
('Orthopedics', 'Bone, joint, and musculoskeletal care'),
('Pediatrics', 'Medical care for infants, children, and adolescents'),
('Radiology', 'Medical imaging and diagnostic services'),
('Laboratory', 'Clinical laboratory and pathology services'),
('Pharmacy', 'Medication management and dispensing');

-- Sample users (passwords should be hashed in real implementation)
INSERT INTO users (username, password_hash, user_type) VALUES
('admin', SHA2('admin123', 256), 4),
('dr.smith', SHA2('doctor123', 256), 2),
('dr.johnson', SHA2('doctor123', 256), 2),
('dr.williams', SHA2('doctor123', 256), 2),
('nurse.brown', SHA2('nurse123', 256), 3),
('patient.doe', SHA2('patient123', 256), 1);

-- Sample doctors
INSERT INTO doctors (user_id, first_name, last_name, specialization, department_id, phone, email, office_room, license_number, years_experience) VALUES
(2, 'John', 'Smith', 'Cardiology', 3, '555-0101', 'j.smith@hospital.com', 'C201', 'MD12345', 15),
(3, 'Sarah', 'Johnson', 'Internal Medicine', 2, '555-0102', 's.johnson@hospital.com', 'IM101', 'MD12346', 10),
(4, 'Michael', 'Williams', 'Emergency Medicine', 1, '555-0103', 'm.williams@hospital.com', 'ER01', 'MD12347', 8);

-- Sample nurses
INSERT INTO nurses (user_id, first_name, last_name, department_id, phone, email, shift_type, license_number) VALUES
(5, 'Lisa', 'Brown', 1, '555-0201', 'l.brown@hospital.com', 'DAY', 'RN12345');

-- Sample patients
INSERT INTO patients (user_id, first_name, last_name, date_of_birth, gender, phone, email, address, emergency_contact, emergency_phone) VALUES
(6, 'Jane', 'Doe', '1985-03-15', 'F', '555-0301', 'jane.doe@email.com', '123 Main St, City, State 12345', 'John Doe', '555-0302');

-- Sample medications
INSERT INTO medications (medication_name, generic_name, manufacturer, dosage_form, strength, description) VALUES
('Lisinopril', 'Lisinopril', 'Generic Pharma', 'Tablet', '10mg', 'ACE inhibitor for high blood pressure'),
('Metformin', 'Metformin HCl', 'Generic Pharma', 'Tablet', '500mg', 'Diabetes medication'),
('Ibuprofen', 'Ibuprofen', 'Generic Pharma', 'Tablet', '200mg', 'Pain reliever and anti-inflammatory');

-- Sample rooms
INSERT INTO rooms (room_number, room_type, department_id, bed_count) VALUES
('101', 'GENERAL', 2, 2),
('102', 'PRIVATE', 2, 1),
('201', 'ICU', 3, 1),
('ER01', 'EMERGENCY', 1, 1);

-- Create indexes for better performance
CREATE INDEX idx_users_username ON users(username);
CREATE INDEX idx_users_type ON users(user_type);
CREATE INDEX idx_appointments_datetime ON appointments(appointment_date, appointment_time);
CREATE INDEX idx_medical_records_patient_date ON medical_records(patient_id, record_date);

-- Create views for common queries
CREATE VIEW patient_summary AS
SELECT 
    p.patient_id,
    p.first_name,
    p.last_name,
    p.date_of_birth,
    p.gender,
    p.phone,
    p.email,
    COUNT(DISTINCT a.appointment_id) as total_appointments,
    COUNT(DISTINCT mr.record_id) as total_records,
    MAX(a.appointment_date) as last_appointment_date
FROM patients p
LEFT JOIN appointments a ON p.patient_id = a.patient_id
LEFT JOIN medical_records mr ON p.patient_id = mr.patient_id
GROUP BY p.patient_id;

CREATE VIEW doctor_schedule AS
SELECT 
    d.doctor_id,
    CONCAT(d.first_name, ' ', d.last_name) as doctor_name,
    d.specialization,
    a.appointment_date,
    a.appointment_time,
    CONCAT(p.first_name, ' ', p.last_name) as patient_name,
    a.reason,
    a.status
FROM doctors d
LEFT JOIN appointments a ON d.doctor_id = a.doctor_id
LEFT JOIN patients p ON a.patient_id = p.patient_id
WHERE a.appointment_date >= CURDATE()
ORDER BY d.doctor_id, a.appointment_date, a.appointment_time;