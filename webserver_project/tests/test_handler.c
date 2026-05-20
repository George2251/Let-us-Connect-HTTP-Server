#include "unity.h"
#include "http_handler.h"
#include "http_parser.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>  // <-- ADD THIS for socketpair and SOCK_STREAM
#include <sys/un.h>

int fake_sockets[2];

void setUp(void) {
    // Create a local socket pair to simulate a network client connection
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fake_sockets) < 0) {
        perror("Failed to create mock socket pair");
    }
    
    // Create a dummy public directory and file for testing GET requests
    mkdir("public", 0777);
    int fd = open("public/test.html", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd != -1) {
        write(fd, "Hello from the filesystem!", 26);
        close(fd);
    }
}

void tearDown(void) {
    close(fake_sockets[0]);
    close(fake_sockets[1]);
    remove("public/test.html");
    remove("public/post_data.txt");
}

int add_work(struct thread_pool_t *thrd_pl, struct ClientTask *task) {
    (void)thrd_pl; // Silence unused parameter warning
    (void)task;    // Silence unused parameter warning
    return 0;      // Pretend everything went perfectly
}

void test_handler_should_resolve_extensionless_url_by_appending_html(void) {
    // 1. Setup a clean dummy file without extension in our request string
    int fd = open("public/about.html", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd != -1) {
        write(fd, "Welcome to the about page!", 26);
        close(fd);
    }

    char raw_request[] = "GET /about HTTP/1.1\r\nConnection: close\r\n\r\n";
    struct HTTPRequest req = http_request_constructor(raw_request);
    
    handle_filesystem_request(fake_sockets[0], &req);
    
    char response_buffer[2048] = {0};
    read(fake_sockets[1], response_buffer, sizeof(response_buffer) - 1);
    
    // Assert it successfully appended .html behind the scenes and read the content
    TEST_ASSERT_NOT_NULL(strstr(response_buffer, "HTTP/1.1 200 OK"));
    TEST_ASSERT_NOT_NULL(strstr(response_buffer, "Welcome to the about page!"));
    
    http_request_destructor(&req);
    remove("public/about.html");
}
void test_handler_should_return_404_for_non_existent_file(void) {
    char raw_request[] = "GET /this_file_does_not_exist.xyz HTTP/1.1\r\nConnection: close\r\n\r\n";
    struct HTTPRequest req = http_request_constructor(raw_request);
    
    handle_filesystem_request(fake_sockets[0], &req);
    
    char response_buffer[1024] = {0};
    read(fake_sockets[1], response_buffer, sizeof(response_buffer) - 1);
    
    // Check that our error-sending mechanisms caught the missing file cleanly
    TEST_ASSERT_NOT_NULL(strstr(response_buffer, "HTTP/1.1 404 Not Found"));
    TEST_ASSERT_NOT_NULL(strstr(response_buffer, "<h1>404 Not Found</h1>"));
    
    http_request_destructor(&req);
}
void test_handler_should_reject_post_without_content_length_with_411(void) {
    // POST request with a payload but completely missing the Content-Length header field
    char raw_request[] = "POST /submit HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\nusername=attacker";
    struct HTTPRequest req = http_request_constructor(raw_request);
    
    handle_filesystem_request(fake_sockets[0], &req);
    
    char response_buffer[1024] = {0};
    read(fake_sockets[1], response_buffer, sizeof(response_buffer) - 1);
    
    // Assert that the length verification block trips and issues the proper code
    TEST_ASSERT_NOT_NULL(strstr(response_buffer, "HTTP/1.1 411 Length Required"));
    
    http_request_destructor(&req);
}
void test_handler_should_reject_invalid_http_version_with_505(void) {
    char raw_request[] = "GET /index.html HTTP/1.0\r\nConnection: close\r\n\r\n";
    struct HTTPRequest req = http_request_constructor(raw_request);
    
    handle_filesystem_request(fake_sockets[0], &req);
    
    char response_buffer[1024] = {0};
    read(fake_sockets[1], response_buffer, sizeof(response_buffer) - 1);
    
    TEST_ASSERT_NOT_NULL(strstr(response_buffer, "HTTP/1.1 505 HTTP Version Not Supported"));
    
    http_request_destructor(&req);
}
// --- TEST 1: Path Traversal Attack Security ---
void test_handler_should_reject_path_traversal_with_403(void) {
    // A malicious request attempting to escape the public directory
    char raw_request[] = "GET /../private.txt HTTP/1.1\r\nConnection: close\r\n\r\n";
    struct HTTPRequest req = http_request_constructor(raw_request);
    
    // Pass fake_sockets[0] as the client connection
    handle_filesystem_request(fake_sockets[0], &req);
    
    // Read what the handler wrote back to the "client" from fake_sockets[1]
    char response_buffer[1024] = {0};
    read(fake_sockets[1], response_buffer, sizeof(response_buffer) - 1);
    
    // Verify it caught the exploit and sent a 403 Forbidden
    TEST_ASSERT_NOT_NULL(strstr(response_buffer, "HTTP/1.1 403 Forbidden"));
    
    http_request_destructor(&req);
}

// --- TEST 2: Valid GET Request Execution ---
void test_handler_should_serve_existing_html_file(void) {
    char raw_request[] = "GET /test.html HTTP/1.1\r\nConnection: close\r\n\r\n";
    struct HTTPRequest req = http_request_constructor(raw_request);
    
    handle_filesystem_request(fake_sockets[0], &req);
    
    char response_buffer[2048] = {0};
    read(fake_sockets[1], response_buffer, sizeof(response_buffer) - 1);
    
    // Assert we get an OK status and the actual file content back
    TEST_ASSERT_NOT_NULL(strstr(response_buffer, "HTTP/1.1 200 OK"));
    TEST_ASSERT_NOT_NULL(strstr(response_buffer, "Content-Type: text/html"));
    TEST_ASSERT_NOT_NULL(strstr(response_buffer, "Hello from the filesystem!"));
    
    http_request_destructor(&req);
}

// --- TEST 3: POST Data Storage Automation ---
void test_handler_should_save_post_payload_to_file(void) {
    char raw_request[] = 
        "POST /upload HTTP/1.1\r\n"
        "Content-Length: 18\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n\r\n"
        "data=handler_saved";
        
    struct HTTPRequest req = http_request_constructor(raw_request);
    
    handle_filesystem_request(fake_sockets[0], &req);
    
    // Verify the server replied with a 201 Created status
    char response_buffer[1024] = {0};
    read(fake_sockets[1], response_buffer, sizeof(response_buffer) - 1);
    TEST_ASSERT_NOT_NULL(strstr(response_buffer, "HTTP/1.1 201 Created"));
    
    // Verify the handler successfully flushed the payload data to disk
    FILE* file = fopen("public/post_data.txt", "r");
    TEST_ASSERT_NOT_NULL_MESSAGE(file, "post_data.txt was not created by the handler");
    
    char file_content[128] = {0};
    fgets(file_content, sizeof(file_content), file);
    fclose(file);
    
    TEST_ASSERT_NOT_NULL(strstr(file_content, "handler_saved"));
    
    http_request_destructor(&req);
}
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_handler_should_reject_path_traversal_with_403);
    RUN_TEST(test_handler_should_serve_existing_html_file);
    RUN_TEST(test_handler_should_save_post_payload_to_file);
    
    // New validation tests
    RUN_TEST(test_handler_should_resolve_extensionless_url_by_appending_html);
    RUN_TEST(test_handler_should_return_404_for_non_existent_file);
    RUN_TEST(test_handler_should_reject_invalid_http_version_with_505);
    RUN_TEST(test_handler_should_reject_post_without_content_length_with_411);
    
    return UNITY_END();
}