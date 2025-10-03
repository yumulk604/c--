#!/bin/bash

echo "Starting Hospital Management System Demo..."

# Start server in background
./hospital_server &
SERVER_PID=$!

# Wait for server to start
sleep 2

echo "Server started with PID: $SERVER_PID"

# Test 1: Doctor List
echo "=== Test 1: Doctor List ==="
echo "5" | timeout 5s ./hospital_client

echo ""
echo "=== Test 2: Patient Registration ==="
echo -e "1\nTest\nUser\n12345678901\n5551234567\nTest Address\n25\n0\n0" | timeout 10s ./hospital_client

echo ""
echo "=== Test 3: Patient Login and Appointment ==="
echo -e "2\n12345678901\n3\n1000\n2000\n1640995200\nTest appointment\n0" | timeout 10s ./hospital_client

# Clean up
echo "Stopping server..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo "Demo completed!"