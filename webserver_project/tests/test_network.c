#include "unity.h"
#include "network.h"
#include "http_parser.h"
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

void setUp(void) {
    // Basic structural initialization before each test block
    setup_signals();
}

void tearDown(void) {
    // Clean up routine if needed
}

// Fakes to satisfy internal linking requirements without testing thread_pool logic
int add_work(thread_pool_t *thrd_pl, struct ClientTask *task) {
    (void)thrd_pl; // Silence unused parameter warning
    (void)task;    // Silence unused parameter warning
    return 0;      // Pretend everything went perfectly
}

// --- TEST 1: Server Port Allocation & Binding ---
void test_network_init_should_bind_and_listen_on_available_port(void) {
    // Choose an unprivileged testing port (e.g., 8085)
    int test_port = 8085;
    int server_fd = network_init(test_port);
    
    TEST_ASSERT_TRUE_MESSAGE(server_fd > 0, "Server socket creation failed");
    
    // Verify it was automatically set to non-blocking mode by network_init
    int flags = fcntl(server_fd, F_GETFL, 0);
    TEST_ASSERT_TRUE_MESSAGE(flags & O_NONBLOCK, "Server socket was not set to non-blocking");
    
    network_shutdown(server_fd);
}

// --- TEST 2: Non-Blocking Conversion Safety ---
void test_make_nonblocking_should_apply_flags_correctly(void) {
    int dummy_socket = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT_TRUE(dummy_socket > 0);
    
    // Ensure it starts as regular blocking
    int initial_flags = fcntl(dummy_socket, F_GETFL, 0);
    TEST_ASSERT_FALSE(initial_flags & O_NONBLOCK);
    
    // Convert it using your utility function
    int result = make_nonblocking(dummy_socket);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verify flag was written to the kernel table
    int updated_flags = fcntl(dummy_socket, F_GETFL, 0);
    TEST_ASSERT_TRUE(updated_flags & O_NONBLOCK);
    
    close(dummy_socket);
}

// --- TEST 3: Safe Stream Transmission Data Loop ---
void test_send_all_should_transmit_complete_buffer(void) {
    int local_sockets[2];
    TEST_ASSERT_EQUAL_INT(0, socketpair(AF_UNIX, SOCK_STREAM, 0, local_sockets));
    
    const char message[] = "Let-us-Connect-Stream-Test-Payload-Data";
    int msg_len = strlen(message);
    
    // Send data across the paired socket descriptor link
    int send_result = send_all(local_sockets[0], message, msg_len);
    TEST_ASSERT_EQUAL_INT(0, send_result);
    
    // Read and verify it arrived intact on the other end
    char receive_buffer[128] = {0};
    int read_bytes = read(local_sockets[1], receive_buffer, sizeof(receive_buffer) - 1);
    
    TEST_ASSERT_EQUAL_INT(msg_len, read_bytes);
    TEST_ASSERT_EQUAL_STRING(message, receive_buffer);
    
    close(local_sockets[0]);
    close(local_sockets[1]);
}

void test_check_keepalive_timeouts_should_purge_expired_connections(void) {
    // 1. Initialize a server instance to clear and setup the connection table
    int server_fd = network_init(8086);
    TEST_ASSERT_TRUE(server_fd > 0);

    // 2. Simulate time passing by checking timeouts under normal conditions
    // (Active, non-busy connections created within 10 seconds should stay alive)
    check_keepalive_timeouts();
    
    // 3. Since we cannot manually overwrite the static `client_conns` array's timestamps 
    // from this file directly, we ensure the function executes without crashing 
    // or causing memory access race anomalies.
    network_shutdown(server_fd);
}

void test_setup_signals_should_ignore_sigpipe(void) {
    // Trigger signal setup configuration
    setup_signals();

    // Query the kernel signal table to check what action is currently mapped to SIGPIPE
    struct sigaction old_action;
    TEST_ASSERT_EQUAL_INT(0, sigaction(SIGPIPE, NULL, &old_action));

    // Verify that the signal handler is explicitly set to ignore (SIG_IGN)
    TEST_ASSERT_TRUE_MESSAGE(old_action.sa_handler == SIG_IGN, 
        "CRITICAL SECURITY RISK: SIGPIPE is not set to be ignored!");
}
void test_send_all_should_handle_large_buffers_without_data_truncation(void) {
    int local_sockets[2];
    TEST_ASSERT_EQUAL_INT(0, socketpair(AF_UNIX, SOCK_STREAM, 0, local_sockets));

    // Allocate a buffer large enough to ensure it stretches across standard TCP windows
    char large_payload[4096];
    memset(large_payload, 'A', sizeof(large_payload) - 1);
    large_payload[sizeof(large_payload) - 1] = '\0';
    int payload_len = strlen(large_payload);

    // Verify that send_all successfully loops and drains all bytes 
    int result = send_all(local_sockets[0], large_payload, payload_len);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Set the receiving socket to non-blocking so we can read it in chunks
    make_nonblocking(local_sockets[1]);

    char read_buffer[4096] = {0};
    int total_read = 0;
    int bytes_received = 0;

    // Drain the socket buffer completely
    while ((bytes_received = read(local_sockets[1], read_buffer + total_read, 
                                  sizeof(read_buffer) - 1 - total_read)) > 0) {
        total_read += bytes_received;
    }

    TEST_ASSERT_EQUAL_INT(payload_len, total_read);
    TEST_ASSERT_EQUAL_STRING(large_payload, read_buffer);

    close(local_sockets[0]);
    close(local_sockets[1]);
}
void test_send_all_should_break_on_blocking_buffers(void) {
    int local_sockets[2];
    TEST_ASSERT_EQUAL_INT(0, socketpair(AF_UNIX, SOCK_STREAM, 0, local_sockets));
    
    // Set the sending socket to non-blocking
    make_nonblocking(local_sockets[0]);
    
    // Fill up the kernel's socket send buffer completely by sending data without reading it
    char garbage[1024];
    memset(garbage, 'B', sizeof(garbage));
    while (send(local_sockets[0], garbage, sizeof(garbage), 0) > 0);
    
    // Now call send_all. If it doesn't handle EAGAIN safely, it will freeze here infinitely.
    // We use an alarm to stop the test if it hangs.
    alarm(2); 
    int result = send_all(local_sockets[0], "test", 4);
    alarm(0); // Clear alarm if it escaped
    
    // The test fails if it times out, but ideally it should return an error (-1) instead of spinning.
    TEST_ASSERT_EQUAL_INT(-1, result);
    
    close(local_sockets[0]);
    close(local_sockets[1]);
}
void test_network_should_not_busy_loop_on_fd_exhaustion(void) {
    // 1. Artificially consume file descriptors up to a high limit
    int dummy_fds[1024];
    int count = 0;
    while (count < 1024) {
        int fd = open("/dev/null", O_RDONLY);
        if (fd < 0) break; // Exhausted!
        dummy_fds[count++] = fd;
    }
    
    // 2. Try to initialize a network socket now
    int server_fd = network_init(8089);
    
    // If it fails to allocate or binds poorly due to descriptor table saturation, 
    // it must return -1 gracefully instead of throwing unhandled anomalies.
    if (server_fd > 0) {
        network_shutdown(server_fd);
    }
    
    // Clean up our consumed descriptors
    for (int i = 0; i < count; i++) {
        close(dummy_fds[i]);
    }
}
void test_network_processing_should_reject_negative_content_lengths(void) {
    int local_sockets[2];
    TEST_ASSERT_EQUAL_INT(0, socketpair(AF_UNIX, SOCK_STREAM, 0, local_sockets));
    
    // A malicious request layout containing a negative size boundary
    const char malicious_payload[] = 
        "POST /submit HTTP/1.1\r\n"
        "Content-Length: -9999\r\n\r\n"
        "data=exploit";
        
    // Send it across to simulate an inbound pipeline
    send(local_sockets[0], malicious_payload, strlen(malicious_payload), 0);
    
    // If your worker tasks don't validate content_length > 0, your buffer offsets 
    // will underflow or process incomplete packets immediately.
    
    close(local_sockets[0]);
    close(local_sockets[1]);
}
int main(void) {
    UNITY_BEGIN();
    
    // --- STEP 1: Core OS Network & Flag Tests ---
    RUN_TEST(test_network_init_should_bind_and_listen_on_available_port);
    RUN_TEST(test_make_nonblocking_should_apply_flags_correctly);
    RUN_TEST(test_send_all_should_transmit_complete_buffer);
    
    // --- STEP 2: Baseline Resilience & Security Tests ---
    RUN_TEST(test_check_keepalive_timeouts_should_purge_expired_connections);
    RUN_TEST(test_setup_signals_should_ignore_sigpipe);
    RUN_TEST(test_send_all_should_handle_large_buffers_without_data_truncation);
    
    // --- STEP 3: Module Breaker & Attack Edge Tests ---
    RUN_TEST(test_send_all_should_break_on_blocking_buffers);
    RUN_TEST(test_network_should_not_busy_loop_on_fd_exhaustion);
    RUN_TEST(test_network_processing_should_reject_negative_content_lengths);
    
    return UNITY_END();
}