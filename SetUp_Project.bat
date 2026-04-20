@echo off
set PROJECT_NAME=webserver_project

echo Creating project structure for: %PROJECT_NAME%...

:: Create the root directory
mkdir %PROJECT_NAME%
cd %PROJECT_NAME%

:: Create the top-level files
type nul > Makefile
echo // Entry point for the web server > main.cpp

:: Create the include directory and header files (The API Contracts)
echo Setting up include\ (API Contracts)...
mkdir include

:: Loop through and create headers, injecting #pragma once into each
for %%H in (http_types.hpp network.h thread_pool.h http_parser.hpp http_handler.hpp http_serializer.hpp config_logger.hpp) do (
    echo #pragma once > include\%%H
)

:: Create the src directory and subdirectories for the Pairs
echo Setting up src\ for the 3 Pairs...
mkdir src\network
mkdir src\parser
mkdir src\handler
mkdir tests

:: Pair 1: Network & Concurrency
echo Creating Pair 1 files...
echo #include "../../include/network.h" > src\network\network.c
echo #include "../../include/thread_pool.h" > src\network\thread_pool.cpp

:: Pair 2: Parser
echo Creating Pair 2 files...
echo #include "../../include/http_parser.hpp" > src\parser\http_parser.cpp

:: Pair 3: Handler & File System
echo Creating Pair 3 files...
echo #include "../../include/http_handler.hpp" > src\handler\http_handler.cpp
echo #include "../../include/http_serializer.hpp" > src\handler\http_serializer.cpp
echo #include "../../include/config_logger.hpp" > src\handler\config_logger.cpp

echo.
echo Done! Your workspace is ready.
pause