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
TEST_TARGETS = parser handler network logger

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