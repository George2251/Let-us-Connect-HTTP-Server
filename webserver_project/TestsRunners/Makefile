# ==============================================================================
# Makefile - Automated Test Compilation Pipeline (With Parser Added)
# ==============================================================================

# Compilers and Global Architecture Flags
CC      = gcc
CFLAGS  = -Wall -Wextra -g -DTEST
LDFLAGS = -lpthread

# Include Header Search Trees
INCLUDES = -I../include \
           -I../tests/unity \
           -I../../DataStructures/Dictionary \
           -I../../DataStructures/Common \
           -I../../DataStructures/Trees \
           -I../../DataStructures/Lists

# Data Structure Source Common Requirements
DS_SRCS = ../../DataStructures/Dictionary/Dictionary.c \
          ../../DataStructures/Dictionary/Entry.c \
          ../../DataStructures/Trees/BinarySearchTree.c \
          ../../DataStructures/Lists/LinkedList.c \
          ../../DataStructures/Common/Node.c

# Core Shared Framework Source Files
UNITY_SRCS  = ../tests/unity/unity.c
PARSER_SRCS  = ../src/parser/http_parser.c
NETWORK_SRCS = ../src/network/network.c
HANDLER_SRCS = ../src/handler/http_handler.c
LOGGER_SRCS  = ../src/logger/logger.c

# Targeting Specific Active Test Units (Excluding test_main and test_thread_pool)

TEST_TARGETS = datastructures parser handler network logger

CMOCK_DIR = ../tests/cmock/lib
CMOCK_GEN = ruby $(CMOCK_DIR)/cmock.rb

# Directories where generated mock artifacts will live
MOCK_OUT_DIR = main_build_dir/mocks

# Update compiler flags to include the directory containing the newly generated mock headers
CFLAGS += -I$(MOCK_OUT_DIR) -I$(CMOCK_DIR)

# ------------------------------------------------------------------------------
# 6. Entry Server main.c CMock Testing Build Pass
# ------------------------------------------------------------------------------
main:
	@mkdir -p $(MOCK_OUT_DIR)
	
	@echo "--> [CMock] Auto-generating mock source files from headers..."
	$(CMOCK_GEN) -oout_c=$(MOCK_OUT_DIR) ../include/network.h
	$(CMOCK_GEN) -oout_c=$(MOCK_OUT_DIR) ../include/thread_pool.h
	$(CMOCK_GEN) -oout_c=$(MOCK_OUT_DIR) ../include/logger.h
	$(CMOCK_GEN) -oout_c=$(MOCK_OUT_DIR) ../../DataStructures/Dictionary/Dictionary.h

	@echo "--> [GCC] Compiling generated mocks and main entry test runner..."
	$(CC) $(CFLAGS) $(INCLUDES) \
		$(UNITY_SRCS) $(DS_SRCS) \
		$(MOCK_OUT_DIR)/mock_network.c \
		$(MOCK_OUT_DIR)/mock_thread_pool.c \
		$(MOCK_OUT_DIR)/mock_logger.c \
		$(MOCK_OUT_DIR)/mock_Dictionary.c \
		../src/main.c \
		../tests/test_main.c \
		$(LDFLAGS) \
		-o main_build_dir/main_test_runner > main_build_dir/main_build.log 2>&1
# ... (Keep your existing variables and folder definitions the same) ...

# ------------------------------------------------------------------------------
# 5. Core Data Structures Unit Testing Build Pass
# ------------------------------------------------------------------------------
datastructures:
	@mkdir -p datastructures_build_dir
	$(CC) $(CFLAGS) $(INCLUDES) \
		$(UNITY_SRCS) $(DS_SRCS) \
		../tests/test_datastructures.c \
		$(LDFLAGS) \
		-o datastructures_build_dir/datastructures_test_runner > datastructures_build_dir/datastructures_build.log 2>&1
# Default rule compiles everything
all: $(TEST_TARGETS)

# ------------------------------------------------------------------------------
# 1. HTTP Parser Test Compilation
# ------------------------------------------------------------------------------
parser:
	@mkdir -p parser_build_dir
	$(CC) $(CFLAGS) $(INCLUDES) \
		$(UNITY_SRCS) $(DS_SRCS) $(PARSER_SRCS) \
		../tests/test_parser.c \
		$(LDFLAGS) \
		-o parser_build_dir/parser_test_runner > parser_build_dir/parser_build.log 2>&1

# ------------------------------------------------------------------------------
# 2. HTTP Handler Test Compilation
# ------------------------------------------------------------------------------
handler:
	@mkdir -p handler_build_dir
	$(CC) $(CFLAGS) $(INCLUDES) \
		$(UNITY_SRCS) $(DS_SRCS) $(PARSER_SRCS) $(NETWORK_SRCS) $(HANDLER_SRCS) \
		../tests/test_handler.c \
		$(LDFLAGS) \
		-o handler_build_dir/handler_test_runner > handler_build_dir/handler_build.log 2>&1

# ------------------------------------------------------------------------------
# 3. OS Networking Test Compilation
# ------------------------------------------------------------------------------
network:
	@mkdir -p network_build_dir
	$(CC) $(CFLAGS) $(INCLUDES) \
		$(UNITY_SRCS) $(DS_SRCS) $(PARSER_SRCS) $(NETWORK_SRCS) \
		../tests/test_network.c \
		$(LDFLAGS) \
		-o network_build_dir/network_test_runner > network_build_dir/network_build.log 2>&1

# ------------------------------------------------------------------------------
# 4. Thread-Safe Logger Test Compilation
# ------------------------------------------------------------------------------
logger:
	@mkdir -p logger_build_dir
	$(CC) $(CFLAGS) $(INCLUDES) \
		$(UNITY_SRCS) $(LOGGER_SRCS) \
		../tests/test_logger.c \
		$(LDFLAGS) \
		-o logger_test_runner_bin > logger_build_dir/logger_build.log 2>&1
	@mv -f logger_test_runner_bin logger_build_dir/logger_test_runner 2>/dev/null || true

# ------------------------------------------------------------------------------
# Clean rule to wipe out isolated build folders
# ------------------------------------------------------------------------------
clean:
	rm -rf parser_build_dir handler_build_dir network_build_dir logger_build_dir
	@echo "All local test build environments cleaned."