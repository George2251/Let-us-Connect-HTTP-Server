#define _POSIX_C_SOURCE 200809L
#include "unity.h"
#include "../../DataStructures/Dictionary/Dictionary.h"
#include "../include/thread_pool.h"
#include "../include/network.h"
#include "../include/http_handler.h"

#include <stdlib.h>
#include <string.h>

extern int run_server_entry(int argc, char *argv[]);

int mock_logger_init_fail = 0;
int mock_thread_pool_init_fail = 0;
int mock_network_init_fail = 0;

void setUp()
{

    mock_logger_init_fail = 0;
    mock_thread_pool_init_fail = 0;
    mock_network_init_fail = 0;
}

void tearDown()
{
}

void setup_signals()
{
}

int logger_init(const char *filepath)
{

    if (mock_logger_init_fail)
    {
        return -1; // Force a mock logger failure state
    }
    return 0;
}

void logger_destroy()
{
}

int thread_pool_init(thread_pool_t *pool, int num_of_threads, int type, const char *name)
{
    if (mock_thread_pool_init_fail)
    {
        return -1;
    }
    return 0;
}

int thread_pool_destroy(thread_pool_t *pool, int flags)
{

    return 0;
}

int network_init(int port)
{

    if (mock_network_init_fail)
    {
        return -1;
    }
    return 4; // Fake valid socket file descriptor
}

void network_run_server(int server_fd, thread_pool_t *pool, struct Dictionary *routes)
{
}

void network_shutdown(int server_fd)
{
}

void handle_filesystem_request(int client_fd, struct HTTPRequest *req)
{
}

// testing the acceptance of the thread count
void test_main_should_accept_custom_thread_count_argument()
{
    // we are mocking the main function that takes both args the argc and the argv so we mock them exactly
    char **mock_argv = {"./server", "16"};
    int mock_argc = 2;
    int exitCode = run_server_entry(mock_argc, mock_argv);
    // we see if the exit execution is true
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, exitCode, "Check the if condition of threads at main.c ");
}
// testing if it refueses the negative numbers of thread numbers
void test_main_should_fallback_gracefully_on_invalid_thread_count()
{
    char **mock_argv = {"./server", "-4"};
    int mock_argc = 2;
    int exitCode = run_server_entry(mock_argc, mock_argv);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, exitCode, "Check the if condition of threads at main.c ");
}
// checking if it make default initialization of the the threads number
void test_main_should_boot_normally_with_no_arguments_passed()
{
    char **mock_argv = {"./server"};
    int mock_argc = 1;
    int exitCode = run_server_entry(mock_argc, mock_argv);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, exitCode, "Default initialization failed Check the if condition of threads ");
}

void test_main_should_abort_immediately_if_logger_fails_to_initialize()
{
    mock_logger_init_fail = 1; // causing fail on purpose in the main function

    int mock_argc = 1;
    char **mock_argv = {"./server"};
    int exitCode = run_server_entry(mock_argc, mock_argv);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, exitCode, "Logger initializaion failure is not handled Check the logger if condition");
}

void test_main_should_clean_up_logger_and_abort_if_thread_pool_initialization_fails()
{
    mock_thread_pool_init_fail= 1;
    int mock_argc=1;
    char** mock_argv={"./server"};
    int exitCode= run_server_entry(mock_argc,mock_argv);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0,exitCode,"Check the threading creation error handling at the end of the main.c");
}

void test_main_should_exit_with_error_if_network_fails()
{
    mock_network_init_fail =1;
        int mock_argc=1;
    char** mock_argv={"./server"};
    int exitCode= run_server_entry(mock_argc,mock_argv);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0,exitCode,"Check the Network Function error handling at the middle or the end of the main.c");

}


int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_main_should_accept_custom_thread_count_argument);
    RUN_TEST(test_main_should_fallback_gracefully_on_invalid_thread_count);
    RUN_TEST(test_main_should_boot_normally_with_no_arguments_passed);
    RUN_TEST(test_main_should_abort_immediately_if_logger_fails_to_initialize);
    RUN_TEST(test_main_should_clean_up_logger_and_abort_if_thread_pool_initialization_fails);
    RUN_TEST(test_main_should_exit_with_error_if_network_fails);
    return UNITY_END();
}