#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

PROJECT_NAME="webserver_project"

echo "Creating project structure for: $PROJECT_NAME..."

# Create the root directory
mkdir -p "$PROJECT_NAME"
cd "$PROJECT_NAME"

# Create the top-level files
touch Makefile
touch main.cpp
echo "// Entry point for the web server" > main.cpp

# Create the include directory and header files (The API Contracts)
echo "Setting up include/ (API Contracts)..."
mkdir -p include

headers=(
    "http_types.hpp"
    "network.h"
    "thread_pool.h"
    "http_parser.hpp"
    "http_handler.hpp"
    "http_serializer.hpp"
    "config_logger.hpp"
)

for header in "${headers[@]}"; do
    touch "include/$header"
    # Inject pragma once for C++ header guards
    echo "#pragma once" > "include/$header"
done

# Create the src directory and subdirectories for the Pairs
echo "Setting up src/ for the 3 Pairs..."
mkdir -p src/network
mkdir -p src/parser
mkdir -p src/handler
mkdir -p tests

# Pair 1: Network & Concurrency
echo "Creating Pair 1 files..."
touch src/network/network.c
touch src/network/thread_pool.cpp
echo "#include \"../../include/network.h\"" > src/network/network.c
echo "#include \"../../include/thread_pool.h\"" > src/network/thread_pool.cpp

# Pair 2: Parser
echo "Creating Pair 2 files..."
touch src/parser/http_parser.cpp
echo "#include \"../../include/http_parser.hpp\"" > src/parser/http_parser.cpp

# Pair 3: Handler & File System
echo "Creating Pair 3 files..."
touch src/handler/http_handler.cpp
touch src/handler/http_serializer.cpp
touch src/handler/config_logger.cpp

echo "#include \"../../include/http_handler.hpp\"" > src/handler/http_handler.cpp
echo "#include \"../../include/http_serializer.hpp\"" > src/handler/http_serializer.cpp
echo "#include \"../../include/config_logger.hpp\"" > src/handler/config_logger.cpp

echo "Done! Run 'cd $PROJECT_NAME' to enter your new workspace."