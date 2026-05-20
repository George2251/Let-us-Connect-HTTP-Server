# Target Binary Name
TARGET = adc_thread_pool_test_runner

# Compiler Options
CC = gcc
CFLAGS = -Wall -Wextra -g -DTEST

# Search Directory Paths (All resolved relative to testsRunners/)
INC_DIRS = -I../include \
           -I../tests/mocks \
           -I../tests/unity \
           -I../tests/cmock/src \
           -I../tests/cmock/vendor/c_exception/lib

# Linker Dependencies (Critical for multi-threading)
LDFLAGS = -lpthread

# All Core Source Files Required for the Build
SRCS = ../tests/unity/unity.c \
       ../tests/cmock/src/cmock.c \
       ../tests/mocks/Mockfifo_queue.c \
       ../tests/mocks/Mocknetwork.c \
       ../src/network/thread_pool.c \
       ../tests/test_thread_pool.c

# Object File Tracking Mapping
OBJS = $(SRCS:.c=.o)

# Default Rule: Compile and Link everything
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)

# Compile Individual Source Code Files into Object Units
%.o: %.c
	$(CC) $(CFLAGS) $(INC_DIRS) -c $< -o $@

# Housekeeping: Remove compilation artifacts
clean:
	rm -f $(TARGET)
	find .. -name "*.o" -type f -delete

# Execute the resulting test suite binary directly
run: all
	./$(TARGET)

.PHONY: all clean run