# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

# Paths
SRC_DIR = src
BUILD_DIR = build
LIB_DIR = /opt/iot/lib
POCO_DIR = /opt/iot/poco

# Include and library paths for POCO
INCLUDES = -I$(POCO_DIR)/include -Isrc -Isrc/web
LIBS = -L$(LIB_DIR) -lwebserver -L$(POCO_DIR)/lib -lPocoNet -lPocoUtil -lPocoFoundation -lPocoJSON -lpthread
LDFLAGS = -Wl,-rpath,$(POCO_DIR)/lib

# Target executables
SERVER_TARGET = ebikeGateaway
CLIENT_TARGET = ebikeClient

# Source files
SERVER_SRCS = $(wildcard $(SRC_DIR)/ebikeGateaway.cpp)
CLIENT_SRCS = $(wildcard $(SRC_DIR)/ebikeClient.cpp)

# Object files
SERVER_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SERVER_SRCS))
CLIENT_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(CLIENT_SRCS))

# Build rules
all: $(BUILD_DIR) $(SERVER_TARGET) $(CLIENT_TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/web

$(SERVER_TARGET): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS) $(LDFLAGS)

$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean up build files
clean:
	rm -rf $(BUILD_DIR) $(CLIENT_TARGET) $(SERVER_TARGET)
