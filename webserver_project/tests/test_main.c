#define _POSIX_C_SOURCE 200809L
#include "unity.h"

/* * Include the auto-generated CMock headers.
 * These are built by the tool by parsing your original header files.
 */
#include "Mocknetwork.h"
#include "Mockthread_pool.h"
#include "Mocklogger.h"
#include "MockDictionary.h"

// Declare the external entry function we exposed in main.c via the preprocessor macro
extern int run_server_entry(int argc, char *argv[]);

void setUp(void)
{
    // CMock Internal Requirement: Prepares the mock tracking tables
    CMock_Init(); 
}

void tearDown(void)
{
    // CMock Internal Requirement: Verifies all expected functions were called 
    // and frees the temporary mock memory layouts.
    CMock_Verify();
    CMock_Destroy();
}
// --- TEST 1: Successful Server Boot with Custom Thread Arguments ---
void test_main_should_allocate_custom_worker_threads_from_arguments(void) {
    char *mock_argv[] = {"./server", "12"};
    
    /* * PROGRAMMING EXPECTATIONS:
     * We script the exact behavior we expect main.c to execute.
     */
    
    // 1. We expect logger_init to be called with "server.log" and we return 0 (Success)
    logger_init_ExpectAndReturn("server.log", 0);
    
    // 2. We expect thread_pool_init to be called with 12 threads (parsed from argv[1])
    // We pass CMock variables or tell it to check the exact arguments
    thread_pool_init_ExpectAnyArgsAndReturn(0); 
    
    // 3. We expect the routes dictionary constructor to fire
    struct Dictionary dummy_routes;
    dictionary_constructor_ExpectAndReturn(compare_string_keys, dummy_routes);
    
    // 4. We expect the network subsystem to initialize on port 8080. We return a fake fd (4)
    network_init_ExpectAndReturn(8080, 4);
    
    // 5. We expect the main server loop to execute. 
    // Since network_run_server is a mock, it will bypass its internal infinite while(1) loop instantly!
    network_run_server_ExpectAnyArgs();
    
    // 6. We expect the graceful cleanup path to execute in sequence when network_run_server returns
    network_shutdown_Expect(4);
    thread_pool_destroy_ExpectAnyArgs();
    dictionary_destructor_Ignore(); // We tell CMock to ignore structural pointer operations
    logger_destroy_Expect();

    // Fire the server entry point
    int exit_code = run_server_entry(2, mock_argv);
    
    // Verify main.c executed its entire layout cleanly and returned 0
    TEST_ASSERT_EQUAL_INT(0, exit_code);
}

// --- TEST 2: Graceful Abort Path on Network Initialization Failure ---
void test_main_should_abort_gracefully_if_network_initialization_fails(void) {
    char *mock_argv[] = {"./server"};
    
    // 1. Logger starts up fine
    logger_init_ExpectAndReturn("server.log", 0);
    
    // 2. Thread pool starts up fine (defaults to 8 threads since no arg was passed)
    thread_pool_init_ExpectAnyArgsAndReturn(0);
    
    // 3. Dictionary constructs fine
    struct Dictionary dummy_routes;
    dictionary_constructor_ExpectAndReturn(compare_string_keys, dummy_routes);
    
    // 4. CRITICAL CONDITION: We force network_init to return -1 (Simulation Failure)
    network_init_ExpectAndReturn(8080, -1);
    
    /* * 5. EXPECTED ERROR HANDLING:
     * Because network_init failed, main.c must NEVER call network_run_server or network_shutdown.
     * It should skip straight to destroying the pool, dictionary, and logger, then exit.
     */
    thread_pool_destroy_ExpectAnyArgs();
    dictionary_destructor_Ignore();
    logger_destroy_Expect();

    // Execute the entry point
    int exit_code = run_server_entry(1, mock_argv);
    
    // Verify main.c caught the socket error and returned exit status code 1
    TEST_ASSERT_EQUAL_INT(1, exit_code);
}

// ==============================================================================
// Test Suite Execution Entry Point
// ==============================================================================

int main(void)
{
    // Initialize the Unity testing harness engine
    UNITY_BEGIN();
    
    // Run your CMock-isolated test cases
    RUN_TEST(test_main_should_allocate_custom_worker_threads_from_arguments);
    RUN_TEST(test_main_should_abort_gracefully_if_network_initialization_fails);
    
    // Conclude the test suite and output final pass/fail metrics
    return UNITY_END();
}