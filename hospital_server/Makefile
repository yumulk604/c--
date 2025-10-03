CXX=g++
CXXFLAGS=-std=c++11 -Wall -O2

all: hospital_server hospital_client

hospital_server: hospital_server.o
	$(CXX) $(CXXFLAGS) -o hospital_server hospital_server.o

hospital_server.o: hospital_server.cpp packet.h
	$(CXX) $(CXXFLAGS) -c hospital_server.cpp

hospital_client: hospital_client.o
	$(CXX) $(CXXFLAGS) -o hospital_client hospital_client.o

hospital_client.o: hospital_client.cpp packet.h
	$(CXX) $(CXXFLAGS) -c hospital_client.cpp

clean:
	rm -f hospital_server.o hospital_server hospital_client.o hospital_client

.PHONY: all clean
