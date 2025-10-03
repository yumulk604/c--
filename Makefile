CXX=g++
CXXFLAGS=-std=c++11 -Wall -O2 -I/usr/local/include/mysql -L/usr/local/lib/mysql -lmysqlclient

all: hospital_server hospital_client

hospital_server: hospital_server.o database.o
	$(CXX) $(CXXFLAGS) -o hospital_server hospital_server.o database.o

hospital_client: hospital_client.o
	$(CXX) $(CXXFLAGS) -o hospital_client hospital_client.o

hospital_server.o: hospital_server.cpp hospital_packet.h database.h
	$(CXX) $(CXXFLAGS) -c hospital_server.cpp

hospital_client.o: hospital_client.cpp hospital_packet.h
	$(CXX) $(CXXFLAGS) -c hospital_client.cpp

database.o: database.cpp database.h
	$(CXX) $(CXXFLAGS) -c database.cpp

clean:
	rm -f *.o hospital_server hospital_client

.PHONY: all clean