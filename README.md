# Hospital Management System

A comprehensive hospital management system built in C++ with socket communication, inspired by Metin2 server architecture. This system provides patient registration, appointment booking, medical records management, and more.

## Features

- **Patient Management**: Register new patients, view patient information
- **Appointment System**: Book appointments with doctors, view appointment schedules
- **Medical Records**: Add and view medical history (doctor access required)
- **User Authentication**: Multi-role authentication (Patient, Doctor, Nurse, Admin)
- **Doctor Directory**: View available doctors and their specializations
- **Database Integration**: MySQL/MariaDB backend for data persistence
- **Multi-threaded Server**: Handles multiple concurrent client connections

## Architecture

- **Server**: Multi-threaded TCP server handling client connections
- **Client**: Interactive command-line client for system interaction
- **Database**: MySQL/MariaDB with comprehensive hospital schema
- **Protocol**: Custom binary packet protocol for client-server communication

## Prerequisites

### FreeBSD/Linux
```bash
# Install MySQL/MariaDB
pkg install mysql80-server  # FreeBSD
# or
apt-get install mysql-server libmysqlclient-dev  # Ubuntu/Debian

# Install g++ compiler
pkg install gcc  # FreeBSD
# or
apt-get install build-essential  # Ubuntu/Debian
```

### Database Setup
1. Start MySQL service:
   ```bash
   service mysql-server start  # FreeBSD
   # or
   systemctl start mysql  # Linux
   ```

2. Create database and user:
   ```sql
   mysql -u root -p
   CREATE DATABASE hospital_management;
   CREATE USER 'hospital_user'@'localhost' IDENTIFIED BY 'hospital_pass';
   GRANT ALL PRIVILEGES ON hospital_management.* TO 'hospital_user'@'localhost';
   FLUSH PRIVILEGES;
   EXIT;
   ```

3. Import the schema:
   ```bash
   mysql -u hospital_user -p hospital_management < hospital_schema.sql
   ```

## Compilation

### Standard Build
```bash
make clean
make
```

### FreeBSD Specific Build (if needed)
```bash
# If MySQL is installed in non-standard location
export CXXFLAGS="-I/usr/local/include/mysql -L/usr/local/lib/mysql"
make clean
make
```

### Manual Compilation
```bash
# Server
g++ -std=c++11 -Wall -O2 -I/usr/local/include/mysql -L/usr/local/lib/mysql \
    -o hospital_server hospital_server.cpp database.cpp -lmysqlclient -pthread

# Client
g++ -std=c++11 -Wall -O2 -o hospital_client hospital_client.cpp
```

## Usage

### Starting the Server
```bash
# Default port (13000)
./hospital_server

# Custom port
./hospital_server 8080
```

### Running the Client
```bash
# Connect to localhost
./hospital_client

# Connect to remote server
./hospital_client 192.168.1.100 13000
```

## Sample Usage Workflow

1. **Start the server**: `./hospital_server`
2. **Run the client**: `./hospital_client`
3. **Register a new patient** (option 2)
4. **View available doctors** (option 6)
5. **Login as a patient** (option 1) - use email as username, password: "temp123"
6. **Book an appointment** (option 4)
7. **View appointments** (option 5)

### Sample Login Credentials
The system comes with pre-configured users:

- **Admin**: username: `admin`, password: `admin123`, type: 4
- **Doctor**: username: `dr.smith`, password: `doctor123`, type: 2
- **Patient**: username: `patient.doe`, password: `patient123`, type: 1

## Database Schema

The system includes comprehensive tables for:
- Users and authentication
- Patients and their information
- Doctors and specializations
- Appointments and scheduling
- Medical records and history
- Departments and rooms
- Billing and payments
- Medications and prescriptions

## Network Protocol

The system uses a custom binary protocol with packet headers:
- Handshake packets for connection establishment
- Authentication packets for login
- CRUD packets for data operations
- Error handling packets

## Security Features

- Password hashing with SHA-256
- User type validation
- Permission-based access control
- SQL injection prevention with escaped strings
- Connection validation and timeout handling

## Error Handling

- Database connection monitoring
- Client disconnection handling
- Invalid packet detection
- Comprehensive error reporting

## Performance Features

- Multi-threaded server architecture
- Connection pooling ready
- Database indexing for fast queries
- Efficient binary protocol

## Troubleshooting

### Common Issues

1. **Database Connection Failed**
   - Check if MySQL service is running
   - Verify database credentials
   - Ensure database and tables exist

2. **Compilation Errors**
   - Install MySQL development headers
   - Check g++ compiler version (C++11 support required)
   - Verify include and library paths

3. **Connection Refused**
   - Check if server is running
   - Verify port number
   - Check firewall settings

### Debug Mode
Add debug flags during compilation:
```bash
g++ -std=c++11 -Wall -O2 -DDEBUG -g ...
```

## Future Enhancements

- Web-based interface
- REST API endpoints
- Real-time notifications
- Report generation
- Insurance integration
- Mobile application support

## License

This project is open source and available under the MIT License.