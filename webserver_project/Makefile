# ==========================================
# Web Server Compiler Configuration
# ==========================================

# Compilers
CC = gcc
CXX = g++

# Compiler Flags
# -Wall -Wextra: Enable strict warnings
# -Iinclude: Tell the compiler where to find the .h and .hpp files
# -g: Include debugging symbols (so you can use GDB if it crashes)
# -std=c++17: Use modern C++ features for the parser/handler
# -pthread: Required for std::thread in the thread pool
CFLAGS = -Wall -Wextra -Iinclude -g
CXXFLAGS = -Wall -Wextra -Iinclude -std=c++17 -g
LDFLAGS = -pthread

# Target executable name
TARGET = webserver

# Directories
OBJ_DIR = obj

# ==========================================
# Source Files Listing
# ==========================================

# Pair 1's C code
C_SRCS = src/network/network.c

# Everyone else's C++ code
CXX_SRCS = main.cpp \
           src/network/thread_pool.cpp \
           src/parser/http_parser.cpp \
           src/handler/http_handler.cpp \
           src/handler/http_serializer.cpp \
           src/handler/config_logger.cpp

# Map source files to object files in the obj/ directory
C_OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(C_SRCS))
CXX_OBJS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(CXX_SRCS))
OBJS = $(C_OBJS) $(CXX_OBJS)

# ==========================================
# Build Rules
# ==========================================

# Default target when you type 'make'
all: $(TARGET)

# Linking step (Uses g++ because main() is in C++)
$(TARGET): $(OBJS)
	@echo "Linking executable $@"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile C files
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling C: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ files
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling C++: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	@echo "Cleaning up..."
	rm -rf $(OBJ_DIR) $(TARGET)

# Prevent make from confusing rules with actual files named "all" or "clean"
.PHONY: all clean