# Hospital Management System Makefile
CXX=g++
CXXFLAGS=-std=c++11 -Wall -O2 -pthread

# Targets
all: hospital_server

hospital_server: simple_server.o
	$(CXX) $(CXXFLAGS) -o hospital_server simple_server.o

simple_server.o: simple_server.cpp packet.h
	$(CXX) $(CXXFLAGS) -c simple_server.cpp -o simple_server.o

# Clean target
clean:
	rm -f simple_server.o hospital_server

# Install dependencies (for FreeBSD)
install-deps:
	pkg install -y sqlite3 gmake

# Run server
run: hospital_server
	./hospital_server

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: hospital_server

.PHONY: all clean install-deps run debug