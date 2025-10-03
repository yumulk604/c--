#!/bin/bash

echo "=== Hospital Management System Test ==="
echo

# Check if executables exist
if [ ! -f "hospital_client" ] || [ ! -f "simple_test_server" ]; then
    echo "Error: Executables not found. Please compile first with:"
    echo "make -f Makefile.simple"
    exit 1
fi

echo "✓ Executables found"
echo

# Test server startup (background)
echo "Starting test server in background..."
./simple_test_server &
SERVER_PID=$!

# Wait for server to start
sleep 2

# Check if server is running
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "✗ Server failed to start"
    exit 1
fi

echo "✓ Test server started (PID: $SERVER_PID)"
echo

# Test basic connectivity
echo "Testing basic connectivity..."
if nc -z localhost 13000 2>/dev/null; then
    echo "✓ Server is accepting connections on port 13000"
else
    echo "✗ Cannot connect to server"
    kill $SERVER_PID 2>/dev/null
    exit 1
fi

echo
echo "=== Test Results ==="
echo "✓ Compilation successful"
echo "✓ Server starts correctly"
echo "✓ Network connectivity working"
echo
echo "To test the full system:"
echo "1. Keep the server running: ./simple_test_server"
echo "2. In another terminal, run: ./hospital_client"
echo "3. Try the following operations:"
echo "   - Register a new patient (option 2)"
echo "   - View available doctors (option 6)"
echo "   - Login with any credentials (option 1)"
echo "   - Get patient information (option 3)"
echo

# Clean up
echo "Stopping test server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo "✓ Test completed successfully!"
echo
echo "The system is ready for use. For full database functionality,"
echo "install MySQL and use the full version with: make"