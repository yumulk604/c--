-- TimescaleDB Hospital Management System Setup Script
-- Run this script as postgres superuser

-- Create database
CREATE DATABASE hospital_db;

-- Connect to the new database
\c hospital_db;

-- Enable TimescaleDB extension
CREATE EXTENSION IF NOT EXISTS timescaledb;

-- Create patients table
CREATE TABLE IF NOT EXISTS patients (
    id SERIAL PRIMARY KEY,
    name VARCHAR(64) NOT NULL,
    surname VARCHAR(64) NOT NULL,
    phone VARCHAR(16) UNIQUE NOT NULL,
    address VARCHAR(128),
    age SMALLINT NOT NULL,
    gender CHAR(1) CHECK (gender IN ('M', 'F')),
    blood_type VARCHAR(4),
    registration_date TIMESTAMPTZ DEFAULT NOW()
);

-- Create doctors table
CREATE TABLE IF NOT EXISTS doctors (
    id SERIAL PRIMARY KEY,
    name VARCHAR(64) NOT NULL,
    specialization VARCHAR(32) NOT NULL,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- Create appointments table
CREATE TABLE IF NOT EXISTS appointments (
    id SERIAL PRIMARY KEY,
    patient_id INTEGER REFERENCES patients(id) ON DELETE CASCADE,
    doctor_id INTEGER REFERENCES doctors(id) ON DELETE CASCADE,
    day_of_week SMALLINT CHECK (day_of_week >= 0 AND day_of_week <= 6),
    hour SMALLINT CHECK (hour >= 0 AND hour <= 23),
    reason VARCHAR(128),
    appointment_date TIMESTAMPTZ DEFAULT NOW(),
    UNIQUE(doctor_id, day_of_week, hour)
);

-- Create medical_records table
CREATE TABLE IF NOT EXISTS medical_records (
    id SERIAL PRIMARY KEY,
    patient_id INTEGER REFERENCES patients(id) ON DELETE CASCADE,
    doctor_id INTEGER REFERENCES doctors(id) ON DELETE CASCADE,
    diagnosis TEXT,
    treatment TEXT,
    medication VARCHAR(128),
    visit_date TIMESTAMPTZ DEFAULT NOW(),
    follow_up_required BOOLEAN DEFAULT FALSE
);

-- Convert tables to TimescaleDB hypertables
SELECT create_hypertable('appointments', 'appointment_date', 
                        chunk_time_interval => INTERVAL '1 day');

SELECT create_hypertable('medical_records', 'visit_date', 
                        chunk_time_interval => INTERVAL '1 day');

-- Insert sample doctors
INSERT INTO doctors (name, specialization) VALUES
('Dr. Smith', 'Cardiology'),
('Dr. Johnson', 'Neurology'),
('Dr. Williams', 'Pediatrics'),
('Dr. Brown', 'Orthopedics')
ON CONFLICT DO NOTHING;

-- Create indexes for better performance
CREATE INDEX IF NOT EXISTS idx_patients_name ON patients(name);
CREATE INDEX IF NOT EXISTS idx_patients_surname ON patients(surname);
CREATE INDEX IF NOT EXISTS idx_patients_phone ON patients(phone);
CREATE INDEX IF NOT EXISTS idx_appointments_patient_id ON appointments(patient_id);
CREATE INDEX IF NOT EXISTS idx_appointments_doctor_id ON appointments(doctor_id);
CREATE INDEX IF NOT EXISTS idx_medical_records_patient_id ON medical_records(patient_id);

-- Create a user for the application
CREATE USER hospital_user WITH PASSWORD 'hospital_password';

-- Grant necessary permissions
GRANT CONNECT ON DATABASE hospital_db TO hospital_user;
GRANT USAGE ON SCHEMA public TO hospital_user;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO hospital_user;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO hospital_user;

-- Grant permissions for future tables
ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT SELECT, INSERT, UPDATE, DELETE ON TABLES TO hospital_user;
ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT USAGE, SELECT ON SEQUENCES TO hospital_user;

-- Display setup completion message
SELECT 'TimescaleDB Hospital Management System setup completed successfully!' as message;