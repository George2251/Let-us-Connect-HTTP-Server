CC = gcc
CFLAGS = -Wall -Wextra -g -D_GNU_SOURCE
INCLUDES = -I./include -I../DataStructures/Dictionary -I../DataStructures/Lists -I../DataStructures/Common -I../DataStructures/Trees

# Network and Thread Pool source files (Removed safe_file_operation.c)
NET_SRC = src/network/network.c src/network/thread_pool.c src/network/fifo_queue.c

# Parser source files
PARSER_SRC = src/parser/http_parser.c

# Handler source files
HANDLER_SRC = src/handler/http_handler.c src/handler/logger.c

# Data Structure source files
DS_SRC = ../DataStructures/Dictionary/Dictionary.c ../DataStructures/Dictionary/Entry.c ../DataStructures/Lists/LinkedList.c ../DataStructures/Lists/Queue.c ../DataStructures/Common/Node.c ../DataStructures/Trees/BinarySearchTree.c

# Combine all sources including main.c
SRCS = main.c $(NET_SRC) $(PARSER_SRC) $(HANDLER_SRC) $(DS_SRC)

# Output executable name
TARGET = server

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) $(SRCS) -lpthread

clean:
	rm -f $(TARGET)