#include "unity.h"
#include "http_parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void setUp(void) {
    // No mock initialization needed! This is a pure unit test.
}

void tearDown(void) {
    // No mock cleanup needed!
}

// --- TEST 1: Standard GET Request ---
void test_http_parser_should_extract_request_line_correctly(void) {
    char raw_request[] = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
    
    struct HTTPRequest req = http_request_constructor(raw_request);

    char* method = (char*)req.request_line.search(&req.request_line, "method", sizeof("method"));
    char* uri = (char*)req.request_line.search(&req.request_line, "uri", sizeof("uri"));
    char* version = (char*)req.request_line.search(&req.request_line, "http_version", sizeof("http_version"));

    TEST_ASSERT_NOT_NULL_MESSAGE(method, "Method should not be NULL");
    TEST_ASSERT_EQUAL_STRING("GET", method);
    TEST_ASSERT_EQUAL_STRING("/index.html", uri);
    TEST_ASSERT_EQUAL_STRING("HTTP/1.1", version);

    http_request_destructor(&req);
}

// --- TEST 2: Header Lowercasing (Testing your recent fix!) ---
void test_http_parser_should_force_headers_to_lowercase(void) {
    char raw_request[] = "GET / HTTP/1.1\r\nContent-Length: 42\r\nKeep-Alive: timeout=5\r\n\r\n";
    
    struct HTTPRequest req = http_request_constructor(raw_request);

    // Notice we search using lowercase keys!
    char* cl = (char*)req.header_fields.search(&req.header_fields, "content-length", sizeof("content-length"));
    char* ka = (char*)req.header_fields.search(&req.header_fields, "keep-alive", sizeof("keep-alive"));

    TEST_ASSERT_NOT_NULL_MESSAGE(cl, "Failed to find lowercase 'content-length'");
    TEST_ASSERT_EQUAL_STRING("42", cl);
    
    TEST_ASSERT_NOT_NULL_MESSAGE(ka, "Failed to find lowercase 'keep-alive'");
    TEST_ASSERT_EQUAL_STRING("timeout=5", ka);

    http_request_destructor(&req);
}

// --- TEST 3: Form URL-Encoded POST Body ---
void test_http_parser_should_extract_url_encoded_body_fields(void) {
    char raw_request[] = 
        "POST /submit HTTP/1.1\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n\r\n"
        "username=admin&password=12345";
        
    struct HTTPRequest req = http_request_constructor(raw_request);

    // 1. Check if the raw data was saved
    char* raw_data = (char*)req.body.search(&req.body, "data", sizeof("data"));
    TEST_ASSERT_NOT_NULL(raw_data);
    TEST_ASSERT_EQUAL_STRING("username=admin&password=12345", raw_data);

    // 2. Check if it successfully split the key/value pairs
    char* user = (char*)req.body.search(&req.body, "username", sizeof("username"));
    char* pass = (char*)req.body.search(&req.body, "password", sizeof("password"));

    TEST_ASSERT_NOT_NULL_MESSAGE(user, "Failed to parse username from body");
    TEST_ASSERT_EQUAL_STRING("admin", user);
    TEST_ASSERT_EQUAL_STRING("12345", pass);

    http_request_destructor(&req);
}

// --- TEST 4: Empty Request Safety (Anti-Crash Test) ---
void test_http_parser_should_not_crash_on_empty_string(void) {
    char raw_request[] = "";
    
    printf("\n--- Testing the crash ---\n");
    printf("[1] Before constructor - Sending empty string.\n");
    
    struct HTTPRequest req = http_request_constructor(raw_request);

    printf("[2] After constructor - Structure returned successfully!\n");
    printf("    Address of request_line dictionary: %p\n", (void*)&req.request_line);
    printf("    Address of search function pointer: %p\n", (void*)req.request_line.search);

    // Safety guard to catch uninitialized or garbage function pointers early
    if (req.request_line.search == NULL) {
        printf("    [CRITICAL ALERT]: The search function pointer is NULL! Calling it will crash.\n");
    } else {
        printf("    [INFO]: The search function pointer exists. Attempting execution...\n");
    }

    // This is the line where the application falls over
    char* method = (char*)(req.request_line.search(&req.request_line, "method", sizeof("method")));
    
    printf("[3] After search execution - Result: %p\n", (void*)method);
    TEST_ASSERT_NULL_MESSAGE(method, "Method should be NULL for an empty request");

    http_request_destructor(&req);
    printf("---  TEST 4 END ---\n\n");
}
void test_http_parser_header_with_no_value(void) {
    // A colon with nothing after it, followed immediately by the end of headers
    char raw_request[] = "GET / HTTP/1.1\r\nAffected-Header:\r\n\r\n";
    
    struct HTTPRequest req = http_request_constructor(raw_request);
    
    // Ensure it doesn't crash and either ignores the header or stores an empty string Safely
    char* val = (char*)req.header_fields.search(&req.header_fields, "affected-header", sizeof("affected-header"));
    // It should handle this without dropping out with a Segfault!
    
    http_request_destructor(&req);
}
void test_http_parser_multiple_spaces_in_request_line(void) {
    // Maliciously padded spaces
    char raw_request[] = "GET      /weird-uri/test.php     HTTP/1.1\r\nHost: localhost\r\n\r\n";
    
    struct HTTPRequest req = http_request_constructor(raw_request);

    char* method = (char*)req.request_line.search(&req.request_line, "method", sizeof("method"));
    char* uri = (char*)req.request_line.search(&req.request_line, "uri", sizeof("uri"));
    char* version = (char*)req.request_line.search(&req.request_line, "http_version", sizeof("http_version"));

    TEST_ASSERT_EQUAL_STRING("GET", method);
    TEST_ASSERT_EQUAL_STRING("/weird-uri/test.php", uri);
    TEST_ASSERT_EQUAL_STRING("HTTP/1.1", version);

    http_request_destructor(&req);
}

void test_http_parser_header_with_only_spaces(void) {
    char raw_request[] = "GET / HTTP/1.1\r\nX-Empty-Header:             \r\n\r\n";
    
    struct HTTPRequest req = http_request_constructor(raw_request);
    
    char* val = (char*)req.header_fields.search(&req.header_fields, "x-empty-header", sizeof("x-empty-header"));
    // The trimmer must cleanly reduce this to an empty string "" or return NULL safely
    if (val) {
        TEST_ASSERT_EQUAL_STRING("", val);
    }

    http_request_destructor(&req);
}

void test_http_parser_invalid_url_encoded_body(void) {
    char raw_request[] = 
        "POST /submit HTTP/1.1\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n\r\n"
        "this_is_garbage_data_with_no_equal_signs_or_ampersands";
        
    // This will force extract_body to look for key/value separations that don't exist
    struct HTTPRequest req = http_request_constructor(raw_request);

    char* raw_data = (char*)req.body.search(&req.body, "data", sizeof("data"));
    TEST_ASSERT_NOT_NULL(raw_data);
    
    // It should preserve the raw string, but not corrupt the dictionary structure
    http_request_destructor(&req);
}
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_http_parser_should_extract_request_line_correctly);
    RUN_TEST(test_http_parser_should_force_headers_to_lowercase);
    RUN_TEST(test_http_parser_should_extract_url_encoded_body_fields);
    RUN_TEST(test_http_parser_should_not_crash_on_empty_string);
    RUN_TEST(test_http_parser_header_with_no_value);
    RUN_TEST(test_http_parser_multiple_spaces_in_request_line);
    RUN_TEST(test_http_parser_header_with_only_spaces);
    RUN_TEST(test_http_parser_invalid_url_encoded_body);

    return UNITY_END();
}