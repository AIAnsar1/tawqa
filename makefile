# TAWQA Makefile
# Modern C++23 build configuration

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -O2 -g
LDFLAGS = 

# Source files
SOURCES = tawqa.cc tawqa_getopt.cc tawqa_doexec.cc
OBJECTS = $(SOURCES:.cc=.o)
TARGET = tawqa

# Default target
.PHONY: all clean install help

all: $(TARGET)

# Build main executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# Compile source files
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Dependencies
tawqa.o: tawqa.cc tawqa_generic.hh tawqa_getopt.hh
tawqa_getopt.o: tawqa_getopt.cc tawqa_getopt.hh tawqa_generic.hh
tawqa_doexec.o: tawqa_doexec.cc tawqa_generic.hh

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Install (optional)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Help
help:
	@echo "TAWQA Build System"
	@echo "Available targets:"
	@echo "  all     - Build the main executable (default)"
	@echo "  clean   - Remove build artifacts"
	@echo "  install - Install to /usr/local/bin"
	@echo "  help    - Show this help message"
	@echo ""
	@echo "Build with: make"
	@echo "Clean with: make clean"