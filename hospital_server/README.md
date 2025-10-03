Hospital Server (Example)

A simple TCP server in C++17 for a hospital-like application, inspired by the Metin2 handshake example but using a text-based protocol and SQLite for data persistence.

Features:
- TCP handshake (version + timestamp)
- Commands: PING, ADD_PATIENT, LIST_PATIENTS, ADD_APPOINTMENT, LIST_APPOINTMENTS, HELP, QUIT
- SQLite DB: patients(id, name, dob), appointments(id, patient_id, when_ts, notes)

Build:
- Dependencies: g++, sqlite3, pthreads
- Run: `make -C hospital_server`

Usage:
- Start: `./bin/hospital_server 0.0.0.0 13000 hospital.db`
- Connect via netcat: `nc 127.0.0.1 13000`
- Example session:
```
HELLO 1
PING
ADD_PATIENT John_Doe 1990-01-01
LIST_PATIENTS
ADD_APPOINTMENT 1 2025-10-03T09:00:00 Checkup
LIST_APPOINTMENTS 1
QUIT
```
