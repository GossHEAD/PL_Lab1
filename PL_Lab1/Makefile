CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -I include
LDFLAGS =

SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

SOURCES = $(SRC_DIR)/lexer.cpp $(SRC_DIR)/parser.cpp $(SRC_DIR)/dot_export.cpp $(SRC_DIR)/json_export.cpp $(SRC_DIR)/main.cpp
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
TARGET = $(BUILD_DIR)/parser

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

test: $(TARGET)
	@echo "=== Running test ==="
	./$(TARGET) test/example.v4 test/example.dot
	@echo "=== Done ==="
