# ==============================================================================
# Master Test Suite Makefile - HTTP Web Server Testing Infrastructure
# ==============================================================================

# 1. Compiler and Flag Customizations
CC      = gcc
CFLAGS  = -Wall -Wextra -g -DTEST
LDFLAGS = -lpthread

# 2. Complete Header Inclusion Mapping Targets
INCLUDES = -I../include \
           -I../tests/unity \
           -I../../DataStructures/Dictionary \
           -I../../DataStructures/Common \
           -I../../DataStructures/Trees \
           -I../../DataStructures/Lists

# 3. Aggregated Reusable Component Sources
UNITY_SRCS = ../tests/unity/unity.c

DS_SRCS    = ../../DataStructures/Dictionary/Dictionary.c \
             ../../DataStructures/Dictionary/Entry.c \
             ../../DataStructures/Trees/BinarySearchTree.c \
             ../../DataStructures/Lists/LinkedList.c \
             ../../DataStructures/Common/Node.c

# 4. Standard Top-Level Automation Master Phonies (datastructures removed)
.PHONY: all clean parser handler network logger main

# Running 'make all' will now only compile the working targets
all: parser handler network logger main

# ------------------------------------------------------------------------------
# SUITE 2: HTTP Header String Parser Isolation Pass
# ------------------------------------------------------------------------------
parser:
	@mkdir -p parser_build_dir
	@echo "--> [GCC] Compiling HTTP String Parser suite..."
	$(CC) $(CFLAGS) $(INCLUDES) \
		$(UNITY_SRCS) \
		$(DS_SRCS) \
		../src/parser/http_parser.c \
		../tests/test_parser.c \
		$(LDFLAGS) \
		-o parser_build_dir/parser_test_runner > parser_build_dir/parser_build.log 2>&1

# ------------------------------------------------------------------------------
# SUITE 3: Static & Route Web Filesystem Handler Pass
# ------------------------------------------------------------------------------
handler:
	@mkdir -p handler_build_dir
	@echo "--> [GCC] Compiling HTTP Filesystem Router Handler suite..."
	$(CC) $(CFLAGS) $(INCLUDES) \
		$(UNITY_SRCS) \
		$(DS_SRCS) \
		../src/parser/http_parser.c \
		../src/network/network.c \
		../src/handler/http_handler.c \
		../tests/test_handler.c \
		$(LDFLAGS) \
		-o handler_build_dir/handler_test_runner > handler_build_dir/handler_build.log 2>&1

# ------------------------------------------------------------------------------
# SUITE 4: Core Operating System Socket Layer Abstraction
# ------------------------------------------------------------------------------
network:
	@mkdir -p network_build_dir
	@echo "--> [GCC] Compiling Low-Level OS Networking Layer suite..."
	$(CC) $(CFLAGS) $(INCLUDES) \
		$(UNITY_SRCS) \
		../src/network/network.c \
		../tests/test_network.c \
		$(LDFLAGS) \
		-o network_build_dir/network_test_runner > network_build_dir/network_build.log 2>&1

# ------------------------------------------------------------------------------
# SUITE 5: Concurrent / Multithreaded Operational Subsystems
# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------
# SUITE 5: Concurrent / Multithreaded Operational Subsystems
# ------------------------------------------------------------------------------
logger:
	@mkdir -p logger_build_dir
	@echo "--> [GCC] Compiling Thread-Safe Data Logger suite..."
	$(CC) $(CFLAGS) $(INCLUDES) \
		$(UNITY_SRCS) \
		$(DS_SRCS) \
		../src/parser/http_parser.c \
		../src/network/network.c \
		../src/logger/logger.c \
		../src/network/thread_pool.c \
		../src/network/fifo_queue.c \
		../tests/test_logger.c \
		$(LDFLAGS) \
		-o logger_build_dir/logger_test_runner > logger_build_dir/logger_build.log 2>&1

# ------------------------------------------------------------------------------
# SUITE 6: Core Server main.c Entry Control Orchestration (Manual Mocks)
# ------------------------------------------------------------------------------
main:
	@mkdir -p main_build_dir
	@echo "--> [GCC] Compiling System Main Entry orchestration suite via Link-Time Mocks..."
	$(CC) $(CFLAGS) $(INCLUDES) \
		$(UNITY_SRCS) \
		$(DS_SRCS) \
		../main.c \
		../tests/test_main.c \
		$(LDFLAGS) \
		-o main_build_dir/main_test_runner > main_build_dir/main_build.log 2>&1

# ------------------------------------------------------------------------------
# Workspace Environmental Clean Up
# ------------------------------------------------------------------------------
clean:
	@echo "--> [Cleanup] Sweeping build logs and purging temporary executable artifacts..."
	rm -rf datastructures_build_dir \
	       parser_build_dir \
	       handler_build_dir \
	       network_build_dir \
	       logger_build_dir \
	       main_build_dir \
	       test_results.txt