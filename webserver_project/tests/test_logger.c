#define _POSIX_C_SOURCE 200809L
#include "unity.h"
#include "logger.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define TEST_LOG_FILE "test_server.log"

void setUp(void) {
    // Ensure the log file doesn't exist before running a test
    remove(TEST_LOG_FILE);
}

void tearDown(void) {
    // Always dismantle the global subsystem state and remove testing files
    logger_destroy();
    remove(TEST_LOG_FILE);
}

// --- TEST 1: Standard File Logging Output ---
void test_logger_should_append_request_data_to_file(void) {
    // Initialize file logging
    TEST_ASSERT_EQUAL_INT(0, logger_init(TEST_LOG_FILE));

    // Fire an example transaction payload
    logger_log_request("192.168.1.5", "GET", "/dashboard", 200, "localhost:8080");

    // Read the file and parse what was written
    FILE *file = fopen(TEST_LOG_FILE, "r");
    TEST_ASSERT_NOT_NULL(file);

    char read_buffer[512] = {0};
    fgets(read_buffer, sizeof(read_buffer), file);
    fclose(file);

    // Verify structural format sub-parts
    TEST_ASSERT_NOT_NULL(strstr(read_buffer, "192.168.1.5 - -"));
    TEST_ASSERT_NOT_NULL(strstr(read_buffer, "\"GET /dashboard\" 200"));
    TEST_ASSERT_NOT_NULL(strstr(read_buffer, "(Host: localhost:8080)"));
}

// --- TEST 2: Safeguards Against Double Initialization ---
void test_logger_init_should_reject_double_initialization(void) {
    // First setup pass should succeed
    TEST_ASSERT_EQUAL_INT(0, logger_init(TEST_LOG_FILE));
    
    // Second initialization layout must return early without crashing or re-allocating
    TEST_ASSERT_EQUAL_INT(0, logger_init("another_file.log"));
}

// --- TEST 3: Null Parameter Fallback Safety Checks ---
void test_logger_should_substitute_null_parameters_with_hyphens(void) {
    TEST_ASSERT_EQUAL_INT(0, logger_init(TEST_LOG_FILE));

    // Call logging engine with complete NULL parameters
    logger_log_request(NULL, NULL, NULL, 500, NULL);

    FILE *file = fopen(TEST_LOG_FILE, "r");
    TEST_ASSERT_NOT_NULL(file);

    char read_buffer[512] = {0};
    fgets(read_buffer, sizeof(read_buffer), file);
    fclose(file);

    // Ensure the fallback values were written properly
    TEST_ASSERT_NOT_NULL(strstr(read_buffer, "- - - ["));
    TEST_ASSERT_NOT_NULL(strstr(read_buffer, "\"- -\" 500 (Host: -)"));
}

// --- TEST 4: Heavy Multi-Threaded Stress Test ---
#define NUM_THREADS 5
#define LOGS_PER_THREAD 20

void* thread_log_worker(void* arg) {
    long thread_id = (long)arg;
    char mock_ip[32];
    snprintf(mock_ip, sizeof(mock_ip), "10.0.0.%ld", thread_id);

    for (int i = 0; i < LOGS_PER_THREAD; i++) {
        logger_log_request(mock_ip, "POST", "/api/data", 201, "production-server");
    }
    return NULL;
}

void test_logger_should_handle_concurrent_writes_without_interleaving(void) {
    TEST_ASSERT_EQUAL_INT(0, logger_init(TEST_LOG_FILE));

    pthread_t threads[NUM_THREADS];

    // Spawn concurrent threads hammering the logger at the same time
    for (long i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_log_worker, (void*)i);
    }

    // Join them back
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Parse output records to verify total entry match count
    FILE *file = fopen(TEST_LOG_FILE, "r");
    TEST_ASSERT_NOT_NULL(file);

    int total_lines = 0;
    char dummy[512];
    while (fgets(dummy, sizeof(dummy), file) != NULL) {
        total_lines++;
    }
    fclose(file);

    // Expected logs = Threads * Cycles per thread
    TEST_ASSERT_EQUAL_INT(NUM_THREADS * LOGS_PER_THREAD, total_lines);
}
void test_logger_should_log_to_stdout_when_filepath_is_null(void) {
    // 1. Set up a pipe to capture stdout bytes
    int stdout_pipe[2];
    TEST_ASSERT_EQUAL_INT(0, pipe(stdout_pipe));

    // Duplicate standard stdout descriptor so we can restore it later
    int saved_stdout = dup(STDOUT_FILENO);

    // Redirect stdout to our pipe's write end
    dup2(stdout_pipe[1], STDOUT_FILENO);

    // 2. Initialize logger with NULL path
    TEST_ASSERT_EQUAL_INT(0, logger_init(NULL));
    logger_log_request("127.0.0.1", "GET", "/stdout-test", 200, "localhost");

    // Flush and restore stdout immediately so Unity logging doesn't break
    fflush(stdout);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
    close(stdout_pipe[1]); // Close write end so read doesn't block

    // 3. Read and verify pipe buffer contents
    char capture_buf[512] = {0};
    read(stdout_pipe[0], capture_buf, sizeof(capture_buf) - 1);
    close(stdout_pipe[0]);

    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "127.0.0.1 - -"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\"GET /stdout-test\" 200"));
}

void test_logger_should_safely_ignore_calls_after_destruction(void) {
    // Initialize and immediately destroy the subsystem to clear states
    TEST_ASSERT_EQUAL_INT(0, logger_init(TEST_LOG_FILE));
    logger_destroy();

    // This call must hit your internal 'if (!logger_initialized)' guard 
    // and return safely without dereferencing or crashing.
    logger_log_request("10.0.0.1", "GET", "/dropped", 200, "site");

    // Verify the file remains empty or unmodified after destruction
    FILE *file = fopen(TEST_LOG_FILE, "r");
    if (file != NULL) {
        char buf[128] = {0};
        char *result = fgets(buf, sizeof(buf), file);
        fclose(file);
        TEST_ASSERT_NULL_MESSAGE(result, "Data was erroneously written after logger was destroyed!");
    }
}
void test_logger_should_preserve_existing_logs_between_restarts(void) {
    // Phase 1: Initialize, write an initial history trace line, and shut down
    TEST_ASSERT_EQUAL_INT(0, logger_init(TEST_LOG_FILE));
    logger_log_request("1.1.1.1", "GET", "/old-history", 200, "cloud");
    logger_destroy();

    // Phase 2: Spin up the logger a second time on the same file path
    TEST_ASSERT_EQUAL_INT(0, logger_init(TEST_LOG_FILE));
    logger_log_request("2.2.2.2", "POST", "/new-history", 201, "cloud");
    logger_destroy();

    // Phase 3: Verify both entries exist sequentially in the file
    FILE *file = fopen(TEST_LOG_FILE, "r");
    TEST_ASSERT_NOT_NULL(file);

    char line1[512] = {0};
    char line2[512] = {0};
    
    fgets(line1, sizeof(line1), file);
    fgets(line2, sizeof(line2), file);
    fclose(file);

    TEST_ASSERT_NOT_NULL(strstr(line1, "/old-history"));
    TEST_ASSERT_NOT_NULL(strstr(line2, "/new-history"));
}
int main(void) {
    UNITY_BEGIN();
    
    // Core test vectors
    RUN_TEST(test_logger_should_append_request_data_to_file);
    RUN_TEST(test_logger_init_should_reject_double_initialization);
    RUN_TEST(test_logger_should_substitute_null_parameters_with_hyphens);
    RUN_TEST(test_logger_should_handle_concurrent_writes_without_interleaving);
    
    // New validation vectors
    RUN_TEST(test_logger_should_log_to_stdout_when_filepath_is_null);
    RUN_TEST(test_logger_should_safely_ignore_calls_after_destruction);
    RUN_TEST(test_logger_should_preserve_existing_logs_between_restarts);
    
    return UNITY_END();
}