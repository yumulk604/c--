CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 -pthread
LDFLAGS ?=

# Server loads sqlite at runtime via dlopen, so only -ldl is needed
LIBS_SERVER = -ldl
LIBS_CLIENT =

all: hospital_server hospital_client

hospital_server: hospital_server.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS_SERVER) $(LDFLAGS)

hospital_client: hospital_client.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS_CLIENT) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f hospital_server hospital_client *.o hospital.db

.PHONY: all clean
