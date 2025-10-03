CXX ?= g++
CXXFLAGS ?= -std=gnu++17 -Wall -Wextra -O2 -pthread
LDFLAGS ?=
LDLIBS ?= -lsqlite3 -pthread

# If sqlite headers not present, build without sqlite by defining fallback
CHECK_SQLITE := $(shell printf "#include <sqlite3.h>\nint main(){}" | $(CXX) -x c++ - -o /dev/null $(CXXFLAGS) 2>/dev/null && echo yes || echo no)
ifeq ($(CHECK_SQLITE),no)
  CXXFLAGS += -DHOSPITAL_USE_SQLITE=0
  LDLIBS := -pthread
else
  CXXFLAGS += -DHOSPITAL_USE_SQLITE=1
endif

SRC_DIR := src
BIN_DIR := bin
OBJ_DIR := build

SERVER := $(BIN_DIR)/hospital_server
CLIENT := $(BIN_DIR)/hospital_client

SRCS_SERVER := $(SRC_DIR)/server.cpp $(SRC_DIR)/database.h $(SRC_DIR)/protocol.h
SRCS_CLIENT := $(SRC_DIR)/client.cpp $(SRC_DIR)/protocol.h

OBJS_SERVER := $(OBJ_DIR)/server.o
OBJS_CLIENT := $(OBJ_DIR)/client.o

all: $(SERVER) $(CLIENT)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/server.o: $(SRC_DIR)/server.cpp $(SRC_DIR)/database.h $(SRC_DIR)/protocol.h | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/client.o: $(SRC_DIR)/client.cpp $(SRC_DIR)/protocol.h | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SERVER): $(OBJS_SERVER) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(CLIENT): $(OBJS_CLIENT) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
