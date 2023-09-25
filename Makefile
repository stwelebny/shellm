# Compiler to use
CXX = g++

# Compiler flags
CXXFLAGS = -g -std=c++11 -I/opt/homebrew/Cellar/jsoncpp/1.9.5/include

# Linker flags
LDFLAGS = -L/opt/homebrew/Cellar/jsoncpp/1.9.5/lib -lncurses -lcurl -ljsoncpp

# Target executable name
TARGET = chat_client

# Source files
SOURCES = chat_client.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

