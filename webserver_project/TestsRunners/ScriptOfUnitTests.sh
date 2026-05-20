#!/bin/bash

# ==============================================================================
# run_all_tests.sh - Local Test Suite Runner & Report Generator
# ==============================================================================

# Terminal Colors for Summary Display
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' 

# Define Target Output File in the current folder
REPORT_FILE="test_results.txt"

# Clean or initialize the text report file
echo "====================================================================" > "$REPORT_FILE"
echo "                  AUTOMATED TEST SUITE REPORT                       " >> "$REPORT_FILE"
echo "  Generated on: $(date)" >> "$REPORT_FILE"
echo "====================================================================" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

echo -e "${BLUE}====================================================================${NC}"
echo -e "${BLUE}          STARTING CORE HTTP SERVER AUTOMATED TESTING PIPELINE      ${NC}"
echo -e "${BLUE}====================================================================${NC}"
echo ""

# 1. Trigger Clean Compilation via the local Makefile
echo -e "${YELLOW}[1/3] Cleaning and compiling source modules locally...${NC}"
make clean > /dev/null 2>&1
make all > /dev/null 2>&1

echo ""
echo -e "${YELLOW}[2/3] Verifying module compilation logs...${NC}"

check_build_success() {
    local log_file=$1
    local suite_name=$2
    if [ -f "$log_file" ] && [ -s "$log_file" ]; then
        if grep -q "error:" "$log_file"; then
            echo -e "  --> ${RED}FAILED${NC} to compile ${suite_name} module."
            echo -e "[BUILD ERROR] ${suite_name} failed compilation. Check log details inside: ${log_file}" >> "$REPORT_FILE"
            return 1
        fi
    fi
    echo -e "  --> ${GREEN}SUCCESSFULLY${NC} compiled ${suite_name} modules."
    return 0
}

check_build_success "parser_build_dir/parser_build.log" "HTTP Parser"
check_build_success "handler_build_dir/handler_build.log" "HTTP Handler"
check_build_success "network_build_dir/network_build.log" "Networking Abstraction"
check_build_success "logger_build_dir/logger_build.log" "Thread-Safe Logger"

echo ""
echo -e "${YELLOW}[3/3] Executing active test suites (Writing to report file)...${NC}"

SUITES_PASSED=0
SUITES_FAILED=0

run_and_report_suite() {
    local binary_path=$1
    local suite_title=$2

    echo "--------------------------------------------------------------------" >> "$REPORT_FILE"
    echo ">>> SYSTEM SUITE: ${suite_title}" >> "$REPORT_FILE"
    echo "--------------------------------------------------------------------" >> "$REPORT_FILE"

    if [ -f "$binary_path" ]; then
        # Execute binary and redirect raw Unity logs to report file
        ./"$binary_path" >> "$REPORT_FILE" 2>&1
        local exit_code=$?
        
        if [ $exit_code -eq 0 ]; then
            SUITES_PASSED=$((SUITES_PASSED + 1))
        else
            SUITES_FAILED=$((SUITES_FAILED + 1))
        fi
    else
        echo "[ERROR] Executable binary missing at: ${binary_path}" >> "$REPORT_FILE"
        SUITES_FAILED=$((SUITES_FAILED + 1))
    fi
    echo "" >> "$REPORT_FILE"
}

# Run each test runner binary locally
run_and_report_suite "parser_build_dir/parser_test_runner" "HTTP Header String Parser"
run_and_report_suite "handler_build_dir/handler_test_runner" "HTTP Filesystem Handler"
run_and_report_suite "network_build_dir/network_test_runner" "Core OS Network Module"
run_and_report_suite "logger_build_dir/logger_test_runner" "Thread-Safe Data Logger"

echo -e "${BLUE}====================================================================${NC}"
echo -e "${BLUE}                        EXECUTION SUMMARY                           ${NC}"
echo -e "${BLUE}====================================================================${NC}"
echo -e "  Compiled and Ran:  $((SUITES_PASSED + SUITES_FAILED)) Test Suites"
echo -e "  Passed Suites:     ${GREEN}${SUITES_PASSED}${NC}"
if [ $SUITES_FAILED -gt 0 ]; then
    echo -e "  Failed Suites:     ${RED}${SUITES_FAILED}${NC}"
else
    echo -e "  Failed Suites:     ${GREEN}0${NC}"
fi
echo -e "${BLUE}====================================================================${NC}"
echo -e "${YELLOW}Detailed test reports successfully generated at:${NC}"
echo -e "--> ${GREEN}./TestsRunners/${REPORT_FILE}${NC}"
echo ""