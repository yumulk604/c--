#!/bin/bash

echo "=== Hospital Management System Demo ==="
echo

# Check if simple version is compiled
if [ ! -f "simple_test_server" ] || [ ! -f "hospital_client" ]; then
    echo "Compiling simple test version..."
    make -f Makefile.simple
    if [ $? -ne 0 ]; then
        echo "Compilation failed!"
        exit 1
    fi
    echo "✓ Compilation successful"
fi

echo "This demo will show you how the Hospital Management System works."
echo

echo "Step 1: Starting the test server..."
echo "The server will run in the background and handle client connections."
echo

# Start server in background and capture PID
./simple_test_server &
SERVER_PID=$!

# Give server time to start
sleep 2

echo "✓ Server started (PID: $SERVER_PID)"
echo

echo "Step 2: You can now test the system manually:"
echo
echo "In another terminal window, run:"
echo "  ./hospital_client"
echo
echo "Try these operations in order:"
echo "  1. View available doctors (option 6)"
echo "  2. Register as a new patient (option 2)"
echo "  3. Login with any credentials (option 1, type 1 for patient)"
echo "  4. Get patient information (option 3)"
echo "  5. Book an appointment (option 4)"
echo
echo "Press Enter when you're done testing..."
read

echo "Stopping the server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo "✓ Demo completed!"
echo
echo "For the full version with database support:"
echo "  1. Run: ./setup.sh"
echo "  2. Compile with: make"
echo "  3. Start with: ./hospital_server"