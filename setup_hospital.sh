#!/bin/bash

echo "=== Hospital Management System Setup ==="
echo ""

# Check if PostgreSQL is installed
if ! command -v psql &> /dev/null; then
    echo "PostgreSQL is not installed. Please install PostgreSQL first."
    echo "On Ubuntu/Debian: sudo apt-get install postgresql postgresql-contrib"
    echo "On CentOS/RHEL: sudo yum install postgresql postgresql-server"
    echo "On FreeBSD: sudo pkg install postgresql13-server"
    exit 1
fi

# Check if TimescaleDB extension is available
echo "Checking TimescaleDB extension..."
if ! psql -U postgres -d postgres -c "SELECT 1 FROM pg_available_extensions WHERE name = 'timescaledb';" | grep -q "1"; then
    echo "TimescaleDB extension is not available. Please install TimescaleDB first."
    echo "Visit: https://docs.timescale.com/install/latest/"
    exit 1
fi

echo "TimescaleDB extension is available."
echo ""

# Create database and setup schema
echo "Setting up database schema..."
if psql -U postgres -f setup_timescaledb.sql; then
    echo "Database setup completed successfully!"
else
    echo "Database setup failed. Please check your PostgreSQL configuration."
    exit 1
fi

echo ""
echo "=== Compiling Hospital Management System ==="

# Check if libpq is available
if ! pkg-config --exists libpq; then
    echo "libpq development files not found. Installing..."
    if command -v apt-get &> /dev/null; then
        sudo apt-get install libpq-dev
    elif command -v yum &> /dev/null; then
        sudo yum install postgresql-devel
    elif command -v pkg &> /dev/null; then
        sudo pkg install postgresql13-client
    else
        echo "Please install PostgreSQL development files manually."
        exit 1
    fi
fi

# Compile the applications
echo "Compiling applications..."
if make all; then
    echo "Compilation completed successfully!"
else
    echo "Compilation failed. Please check your build environment."
    exit 1
fi

echo ""
echo "=== Setup Complete ==="
echo ""
echo "Database Configuration:"
echo "  Host: localhost"
echo "  Port: 5432"
echo "  Database: hospital_db"
echo "  User: hospital_user"
echo "  Password: hospital_password"
echo ""
echo "Applications:"
echo "  hospital_server  - Main server application"
echo "  hospital_client  - Simple client for testing"
echo "  hospital_admin   - Administrative interface"
echo ""
echo "To start the server:"
echo "  ./hospital_server"
echo ""
echo "To run the admin interface:"
echo "  ./hospital_admin"
echo ""
echo "To test with the client:"
echo "  ./hospital_client"