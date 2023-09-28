# Compiler to use

CXX = g++

# Compiler flags
CXXFLAGS = -g -std=c++11 $(shell pkg-config --cflags jsoncpp)

# Linker flags
LDFLAGS = $(shell pkg-config --libs jsoncpp) -lcurl

# Target executable name
TARGET = chat_client

# Source files
SOURCES = chat_client.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)
CXX = g++

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

