# ==========================================
# Compiler Configurations
# ==========================================
CC = gcc
CFLAGS = -Wall -Wextra -g -pthread

# Name of your final executable output (sits directly in the main root folder)
TARGET = web_server

# The dedicated directory for object files
OBJ_DIR = obj

# List of your project source filenames - NOW INCLUDING main.c
SRCS = main.c http_parser.c fifo_queue.c network.c thread_pool.c http_handler.c logger.c

# Automatically maps all source files to the obj/ directory as .o files
# This guarantees main.c compiles specifically into obj/main.o
OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))

# AUTOMATIC SEARCH PATH (VPATH)
# Dynamically tracks down where main.c and the rest of your source files are located
VPATH = $(sort $(dir $(shell find . -name "*.c" ! -path "./$(OBJ_DIR)/*")))

# ==========================================
# Build Rules
# ==========================================

.PHONY: all clean remove_rogue_objs

# Default target: Sweeps out bad objects first, then links your app
all: remove_rogue_objs $(TARGET)

# Final Link Step: Links everything together and saves the binary in the root directory
$(TARGET): $(OBJS)
	@echo "Linking final executable in main directory: $(TARGET)"
	$(CC) $(CFLAGS) $^ -o $@

# Compilation Step: Forces all generated intermediate .o files into the obj/ folder
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@echo "Compiling $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

# Ensures the obj/ directory exists before compilation cycles begin
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# ==========================================
# Strict Object Cleanup Rules
# ==========================================

# Deletes any .o file NOT inside the designated obj/ folder
remove_rogue_objs:
	@echo "Sweeping for stray .o files outside of '$(OBJ_DIR)'..."
	@find . -type f -name "*.o" ! -path "./$(OBJ_DIR)/*" -delete 2>/dev/null || true

# Complete deep clean
clean:
	@echo "Performing deep clean..."
	rm -rf $(OBJ_DIR) $(TARGET)
	@find . -type f -name "*.o" -delete 2>/dev/null || true
	@echo "Clean complete."