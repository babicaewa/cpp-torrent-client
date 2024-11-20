CXX = g++
CXXFLAGS = -std=c++20 -I./src -pthread
LDFLAGS = -lssl -lcrypto -lcurl

TARGET = main

SRC_DIR = src
SRC_FILES = \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/bencode/bdecode.cpp \
	$(SRC_DIR)/sha1/sha1.cpp \
	$(SRC_DIR)/trackerInfo/trackerComm.cpp \
	$(SRC_DIR)/peerComm/peerComm.cpp \
	$(SRC_DIR)/fileBuilder/fileBuilder.cpp \
	$(SRC_DIR)/utils/utils.cpp \
	$(SRC_DIR)/bitfield/bitfield.cpp \
	$(SRC_DIR)/handshake/handshake.cpp \
	$(SRC_DIR)/peerComm/messages.cpp

OBJ_FILES = $(SRC_FILES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_FILES) $(TARGET)

