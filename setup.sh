#!/bin/bash

# Hospital Management System Setup Script
# Compatible with FreeBSD and Linux

echo "=== Hospital Management System Setup ==="
echo

# Detect OS
OS=$(uname -s)
echo "Detected OS: $OS"

# Function to install packages on FreeBSD
install_freebsd() {
    echo "Installing packages for FreeBSD..."
    
    # Check if pkg is available
    if ! command -v pkg &> /dev/null; then
        echo "Error: pkg command not found. Please install pkg first."
        exit 1
    fi
    
    # Install MySQL server and client
    echo "Installing MySQL server..."
    sudo pkg install -y mysql80-server mysql80-client
    
    # Install g++ compiler
    echo "Installing g++ compiler..."
    sudo pkg install -y gcc
    
    # Enable MySQL service
    echo "Enabling MySQL service..."
    sudo sysrc mysql_enable="YES"
    
    # Start MySQL service
    echo "Starting MySQL service..."
    sudo service mysql-server start
}

# Function to install packages on Linux
install_linux() {
    echo "Installing packages for Linux..."
    
    # Detect Linux distribution
    if [ -f /etc/debian_version ]; then
        # Debian/Ubuntu
        echo "Detected Debian/Ubuntu system"
        sudo apt-get update
        sudo apt-get install -y mysql-server libmysqlclient-dev build-essential
        sudo systemctl start mysql
        sudo systemctl enable mysql
    elif [ -f /etc/redhat-release ]; then
        # RedHat/CentOS/Fedora
        echo "Detected RedHat/CentOS/Fedora system"
        sudo yum install -y mysql-server mysql-devel gcc-c++ make
        sudo systemctl start mysqld
        sudo systemctl enable mysqld
    else
        echo "Unsupported Linux distribution. Please install MySQL and g++ manually."
        exit 1
    fi
}

# Install packages based on OS
case "$OS" in
    FreeBSD)
        install_freebsd
        ;;
    Linux)
        install_linux
        ;;
    *)
        echo "Unsupported operating system: $OS"
        echo "Please install MySQL server and g++ compiler manually."
        exit 1
        ;;
esac

# Wait for MySQL to start
echo "Waiting for MySQL to start..."
sleep 5

# Check if MySQL is running
if ! pgrep -x "mysqld" > /dev/null; then
    echo "Warning: MySQL does not appear to be running."
    echo "Please start MySQL manually and run this script again."
    exit 1
fi

echo "MySQL is running."

# Set up database
echo
echo "=== Database Setup ==="
echo "Please enter MySQL root password (press Enter if no password is set):"
read -s MYSQL_ROOT_PASS

# Create database and user
echo "Creating database and user..."
mysql -u root -p"$MYSQL_ROOT_PASS" << EOF
CREATE DATABASE IF NOT EXISTS hospital_management;
CREATE USER IF NOT EXISTS 'hospital_user'@'localhost' IDENTIFIED BY 'hospital_pass';
GRANT ALL PRIVILEGES ON hospital_management.* TO 'hospital_user'@'localhost';
FLUSH PRIVILEGES;
EOF

if [ $? -eq 0 ]; then
    echo "Database and user created successfully."
else
    echo "Error creating database. Please check your MySQL root password and try again."
    exit 1
fi

# Import schema
echo "Importing database schema..."
if [ -f "hospital_schema.sql" ]; then
    mysql -u hospital_user -p'hospital_pass' hospital_management < hospital_schema.sql
    if [ $? -eq 0 ]; then
        echo "Database schema imported successfully."
    else
        echo "Error importing schema. Please check the hospital_schema.sql file."
        exit 1
    fi
else
    echo "Warning: hospital_schema.sql not found. Please import it manually."
fi

# Compile the application
echo
echo "=== Compilation ==="
echo "Compiling hospital management system..."

# Clean previous builds
make clean 2>/dev/null

# Compile
if make; then
    echo "Compilation successful!"
else
    echo "Compilation failed. Please check the error messages above."
    exit 1
fi

# Create startup scripts
echo
echo "=== Creating Startup Scripts ==="

# Server startup script
cat > start_server.sh << 'EOF'
#!/bin/bash
echo "Starting Hospital Management Server..."
echo "Press Ctrl+C to stop the server"
./hospital_server
EOF

chmod +x start_server.sh

# Client startup script
cat > start_client.sh << 'EOF'
#!/bin/bash
echo "Starting Hospital Management Client..."
echo "Connecting to localhost:13000"
./hospital_client
EOF

chmod +x start_client.sh

echo "Created start_server.sh and start_client.sh"

# Display completion message
echo
echo "=== Setup Complete ==="
echo
echo "Hospital Management System has been set up successfully!"
echo
echo "To start the system:"
echo "1. Start the server: ./start_server.sh"
echo "2. In another terminal, start the client: ./start_client.sh"
echo
echo "Default login credentials:"
echo "  Admin    - username: admin,     password: admin123,     type: 4"
echo "  Doctor   - username: dr.smith,  password: doctor123,    type: 2"
echo "  Patient  - username: patient.doe, password: patient123, type: 1"
echo
echo "You can also register new patients through the client interface."
echo
echo "Database connection details:"
echo "  Host: localhost"
echo "  Database: hospital_management"
echo "  Username: hospital_user"
echo "  Password: hospital_pass"
echo
echo "For more information, see README.md"
echo