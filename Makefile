CXX=g++
CXXFLAGS=-std=c++11 -Wall -O2
PGFLAGS=-lpq

all: hospital_server hospital_client hospital_admin

hospital_server: hospital_server.o hospital_db.o
	$(CXX) $(CXXFLAGS) -o hospital_server hospital_server.o hospital_db.o $(PGFLAGS)

hospital_client: hospital_client.o
	$(CXX) $(CXXFLAGS) -o hospital_client hospital_client.o

hospital_admin: hospital_admin.o hospital_db.o
	$(CXX) $(CXXFLAGS) -o hospital_admin hospital_admin.o hospital_db.o $(PGFLAGS)

hospital_server.o: hospital_server.cpp hospital_packet.h hospital_db.h
	$(CXX) $(CXXFLAGS) -c hospital_server.cpp

hospital_client.o: hospital_client.cpp hospital_packet.h
	$(CXX) $(CXXFLAGS) -c hospital_client.cpp

hospital_db.o: hospital_db.cpp hospital_db.h
	$(CXX) $(CXXFLAGS) -c hospital_db.cpp

hospital_admin.o: hospital_admin.cpp hospital_db.h
	$(CXX) $(CXXFLAGS) -c hospital_admin.cpp

clean:
	rm -f *.o hospital_server hospital_client hospital_admin

.PHONY: all clean