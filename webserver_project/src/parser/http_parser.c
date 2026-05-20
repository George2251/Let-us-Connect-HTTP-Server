#include "http_parser.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Forward declarations
void extract_request_line_fields(struct HTTPRequest *request, char *request_line);
void extract_header_fields(struct HTTPRequest *request, char *header_fields);
void extract_body(struct HTTPRequest *request, char *body);
char *trim_whitespace(char *str);

struct HTTPRequest http_request_constructor(char *request_string) {

    struct HTTPRequest request;
    
    // 1. ALWAYS initialize dictionaries first to prevent segfaults during destruction
    request.request_line = dictionary_constructor(compare_string_keys);
    request.header_fields = dictionary_constructor(compare_string_keys);
    request.body = dictionary_constructor(compare_string_keys);

    // Safe return on empty input. (The initialized dictionaries will safely be freed by the destructor)
// 2. SAFE RETURN: If the input string is NULL or completely empty,
    // return the cleanly initialized structure right away!
    if (!request_string || request_string[0] == '\0') {
        return request; 
    }

    // 2. Make a single, safe mutable copy of the entire request
    char* requested = malloc(strlen(request_string) + 1);
    if (!requested) return request; // Fail gracefully if out of memory
    strcpy(requested, request_string);

    // 3. Safely locate the HTTP Body boundary (\r\n\r\n) without using strtok
    char* body_start = strstr(requested, "\r\n\r\n");
    char* header_part = requested;
    char* body_part = NULL;

    if (body_start) {
        *body_start = '\0';         // Terminate the header section
        body_part = body_start + 4; // The body begins exactly 4 bytes after the \r
    }

    // 4. Safely locate the end of the Request Line (\r\n)
    char* headers_start = strstr(header_part, "\r\n");
    char* request_line_part = header_part;

    if (headers_start) {
        *headers_start = '\0';      // Terminate the request line
        headers_start += 2;         // Headers begin exactly 2 bytes after
    }

    // 5. Extract fields safely based on pointer lengths
    if (request_line_part && strlen(request_line_part) > 0) {
        extract_request_line_fields(&request, request_line_part);
    }
    
    if (headers_start && strlen(headers_start) > 0) {
        extract_header_fields(&request, headers_start);
    }

    if (body_part && strlen(body_part) > 0) {
        extract_body(&request, body_part);
    }

    free(requested);
    return request;
}

void http_request_destructor(struct HTTPRequest *request) {
    if(!request) return;
    dictionary_destructor(&request->request_line);
    dictionary_destructor(&request->header_fields);
    dictionary_destructor(&request->body);
}

// Utility: Safely trim leading and trailing whitespace from strings
char *trim_whitespace(char *str) {
    if (!str) return NULL;
    
    // Trim leading space
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str; // All spaces?
    
    // Trim trailing space
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    // Write new null terminator
    end[1] = '\0';
    return str;
}

void extract_request_line_fields(struct HTTPRequest *request, char *request_line) {
    char *saveptr;
    char *method = strtok_r(request_line, " ", &saveptr);
    char *uri = strtok_r(NULL, " ", &saveptr);
    char *http_version = strtok_r(NULL, " ", &saveptr);
    
    if (method) request->request_line.insert(&request->request_line, "method", sizeof("method"), method, strlen(method) + 1);
    if (uri) request->request_line.insert(&request->request_line, "uri", sizeof("uri"), uri, strlen(uri) + 1);
    if (http_version) request->request_line.insert(&request->request_line, "http_version", sizeof("http_version"), http_version, strlen(http_version) + 1);
}

void extract_header_fields(struct HTTPRequest *request, char *header_fields) {
    char *saveptr;
    char *line = strtok_r(header_fields, "\r\n", &saveptr);
    
    while (line) {
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0'; 
            char *key = trim_whitespace(line);
            char *value = trim_whitespace(colon + 1);
            
            if (key && value && strlen(key) > 0) {
                // CRITICAL FIX: Force all header keys to lowercase before saving to the dictionary
                for (int i = 0; key[i]; i++) {
                    key[i] = tolower((unsigned char)key[i]);
                }
                request->header_fields.insert(&request->header_fields, key, strlen(key) + 1, value, strlen(value) + 1);
            }
        }
        line = strtok_r(NULL, "\r\n", &saveptr);
    }
}

void extract_body(struct HTTPRequest *request, char *body) {
    request->body.insert(&request->body, "data", sizeof("data"), body, strlen(body) + 1);

    // Because we lowercased the keys above, we only ever need to search for the lowercase string!
    char *content_type = (char *)request->header_fields.search(&request->header_fields, "content-type", sizeof("content-type"));

    if (content_type && strstr(content_type, "application/x-www-form-urlencoded")) {
        char* body_copy = malloc(strlen(body) + 1);
        if (body_copy) {
            strcpy(body_copy, body);
            char *saveptr;
            char *pair = strtok_r(body_copy, "&", &saveptr);
            while (pair) {
                char *eq = strchr(pair, '='); 
                if (eq) {
                    *eq = '\0';
                    char *key = pair;
                    char *value = eq + 1;
                    if (key && value) {
                        request->body.insert(&request->body, key, strlen(key) + 1, value, strlen(value) + 1);
                    }
                }
                pair = strtok_r(NULL, "&", &saveptr);
            }
            free(body_copy);
        }
    }
}