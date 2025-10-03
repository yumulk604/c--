# Hospital Management System - Usage Guide

## Quick Start

### Option 1: Simple Test Version (No Database Required)

This version works immediately without any database setup and stores data in memory.

```bash
# Compile the simple version
make -f Makefile.simple

# Start the test server (in one terminal)
./simple_test_server

# Start the client (in another terminal)
./hospital_client
```

### Option 2: Full Version with MySQL Database

```bash
# Run the setup script (installs MySQL, creates database)
./setup.sh

# Compile the full version
make

# Start the full server
./hospital_server

# Start the client
./hospital_client
```

## Client Interface Guide

When you start the client, you'll see this menu:

```
=== Hospital Management System ===
1. Login
2. Register New Patient
3. Get Patient Information
4. Book Appointment
5. View Appointments
6. View Available Doctors
7. Add Medical Record (Doctors only)
8. View Medical History
9. Disconnect
```

### Basic Workflow

1. **First Time Setup**
   - Choose option 2 to register as a new patient
   - Fill in all required information
   - Note your Patient ID for future reference

2. **Login**
   - Choose option 1 to login
   - For test version: use any username/password
   - For full version: use pre-configured credentials or your email

3. **View Doctors**
   - Choose option 6 to see available doctors
   - Note doctor IDs for booking appointments

4. **Book Appointment**
   - Choose option 4 after logging in
   - Provide patient ID, doctor ID, date, time, and reason

5. **View Your Appointments**
   - Choose option 5 to see scheduled appointments

## Sample Data (Test Version)

The simple test server comes with these pre-loaded doctors:

- **Dr. John Smith** (ID: 1) - Cardiology, Office: C201
- **Dr. Sarah Johnson** (ID: 2) - Internal Medicine, Office: IM101  
- **Dr. Michael Williams** (ID: 3) - Emergency Medicine, Office: ER01

## Sample Data (Full Version)

Pre-configured login credentials:

| Role | Username | Password | User Type |
|------|----------|----------|-----------|
| Admin | admin | admin123 | 4 |
| Doctor | dr.smith | doctor123 | 2 |
| Patient | patient.doe | patient123 | 1 |

## Testing the System

### Manual Test Sequence

1. **Start Server**
   ```bash
   ./simple_test_server  # or ./hospital_server for full version
   ```

2. **Start Client**
   ```bash
   ./hospital_client
   ```

3. **Register New Patient**
   - Choose option 2
   - Enter sample data:
     - First Name: John
     - Last Name: Test
     - Date of Birth: 1990-01-01
     - Gender: M
     - Phone: 555-1234
     - Email: john.test@email.com
     - Address: 123 Test St
     - Emergency Contact: Jane Test
     - Emergency Phone: 555-5678

4. **View Doctors**
   - Choose option 6
   - Leave specialization blank to see all doctors

5. **Login**
   - Choose option 1
   - Username: john.test@email.com (or any username for test version)
   - Password: temp123 (or any password for test version)
   - User Type: 1 (Patient)

6. **Get Patient Info**
   - Choose option 3
   - Enter Patient ID: 2 (or the ID you got from registration)

7. **Book Appointment**
   - Choose option 4
   - Patient ID: 2
   - Doctor ID: 1
   - Date: 2024-10-10
   - Time: 14:30
   - Reason: Regular checkup

## Troubleshooting

### Common Issues

1. **"Connection refused"**
   - Make sure the server is running
   - Check if port 13000 is available
   - Try a different port: `./simple_test_server 8080`

2. **"Compilation failed"**
   - Install g++ compiler: `pkg install gcc` (FreeBSD) or `apt install build-essential` (Ubuntu)
   - For full version: install MySQL development headers

3. **"Database connection failed"**
   - Make sure MySQL is running
   - Check database credentials in hospital_server.cpp
   - Run the setup script: `./setup.sh`

4. **"Invalid packet"**
   - Make sure client and server versions match
   - Restart both client and server

### Debug Mode

To enable debug output, compile with:
```bash
g++ -DDEBUG -g -std=c++11 -Wall -O2 ...
```

## Architecture Overview

### Network Protocol

The system uses a custom binary protocol:

1. **Handshake**: Client and server exchange random numbers
2. **Authentication**: Username/password verification
3. **Commands**: Various packet types for different operations
4. **Responses**: Success/failure responses with data

### Data Flow

```
Client -> Handshake -> Server
Client <- Handshake <- Server
Client -> Login -> Server
Client <- Login Success <- Server
Client -> Command -> Server
Client <- Response <- Server
```

### Security Features

- Password hashing (SHA-256 in full version)
- User type validation
- SQL injection prevention
- Connection validation

## Performance Notes

- Server handles multiple concurrent clients
- Each client runs in a separate thread
- Database connections are managed per operation
- Memory usage scales with number of concurrent clients

## Extending the System

### Adding New Packet Types

1. Add new header to `PacketHeader` enum in `hospital_packet.h`
2. Define packet structure in `hospital_packet.h`
3. Add handler in server's main loop
4. Implement client-side sending logic

### Adding New Database Tables

1. Add table to `hospital_schema.sql`
2. Create corresponding struct in `hospital_packet.h`
3. Add database methods in `database.h` and `database.cpp`
4. Implement server handlers

### Customization

- Change default port in source files
- Modify database connection parameters
- Add new user types or permissions
- Extend packet structures for more data

## Production Deployment

For production use, consider:

1. **Security**
   - Use SSL/TLS for network communication
   - Implement proper authentication tokens
   - Add rate limiting and input validation

2. **Database**
   - Use connection pooling
   - Implement proper backup strategies
   - Add database replication

3. **Monitoring**
   - Add logging and metrics
   - Implement health checks
   - Monitor resource usage

4. **Scalability**
   - Consider load balancing
   - Implement caching layers
   - Use asynchronous I/O for better performance