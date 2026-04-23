#!/bin/bash

# Define the project root
PROJECT_NAME="webserver_project"

echo "Scaffolding project: $PROJECT_NAME..."

# 1. Create directory structure
mkdir -p "$PROJECT_NAME/include"
mkdir -p "$PROJECT_NAME/src/network"
mkdir -p "$PROJECT_NAME/src/parser"
mkdir -p "$PROJECT_NAME/src/handler"

# 2. Create root level files
touch "$PROJECT_NAME/Makefile"
touch "$PROJECT_NAME/main.c"

# 3. Create header files in include/
touch "$PROJECT_NAME/include/http_types.h"
touch "$PROJECT_NAME/include/network.h"
touch "$PROJECT_NAME/include/thread_pool.h"
touch "$PROJECT_NAME/include/http_parser.h"
touch "$PROJECT_NAME/include/http_handler.h"

# 4. Create source files in src/ subdirectories
touch "$PROJECT_NAME/src/network/network.c"
touch "$PROJECT_NAME/src/network/thread_pool.c"
touch "$PROJECT_NAME/src/parser/http_parser.c"
touch "$PROJECT_NAME/src/handler/http_handler.c"
touch "$PROJECT_NAME/src/handler/logger.c"

echo "Structure created successfully."
ls -R "$PROJECT_NAME"
