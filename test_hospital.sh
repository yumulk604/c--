#!/bin/bash

echo "=== Hospital Management System Test ==="
echo ""

# Check if executables exist
if [ ! -f "./hospital_server" ]; then
    echo "Error: hospital_server not found. Please run 'make all' first."
    exit 1
fi

if [ ! -f "./hospital_client" ]; then
    echo "Error: hospital_client not found. Please run 'make all' first."
    exit 1
fi

echo "Starting hospital server in background..."
./hospital_server &
SERVER_PID=$!

# Wait a moment for server to start
sleep 2

echo "Server started with PID: $SERVER_PID"
echo ""
echo "You can now run the client in another terminal:"
echo "  ./hospital_client"
echo ""
echo "Or test the server with netcat:"
echo "  echo -e '\x01' | nc localhost 13000"
echo ""
echo "To stop the server, run:"
echo "  kill $SERVER_PID"
echo ""

# Keep the script running
echo "Press Ctrl+C to stop the server and exit this script"
wait $SERVER_PID