#include "http_parser.h"
#include "../../DataStructures/Lists/Queue.h"
#include <string.h>
#include <stdlib.h>

void extract_request_line_fields(struct HTTPRequest *request, char *request_line);
void extract_header_fields(struct HTTPRequest *request, char *header_fields);
void extract_body(struct HTTPRequest *request, char *body);

struct HTTPRequest http_request_constructor(char *request_string)
{
    struct HTTPRequest request;
    request.request_line = dictionary_constructor(compare_string_keys);
    request.header_fields = dictionary_constructor(compare_string_keys);
    request.body = dictionary_constructor(compare_string_keys);

    if (request_string == NULL) return request;

    char *requested = strdup(request_string);
    char *saveptr;

    char *body_ptr = strstr(requested, "\r\n\r\n");
    char *header_ptr = strstr(requested, "\r\n");

    // Split request line
    char *request_line = strtok_r(requested, "\r\n", &saveptr);
    
    // Headers and body are handled via pointers to maintain thread safety
    char *header_fields = (header_ptr) ? header_ptr + 2 : NULL;
    char *body = (body_ptr) ? body_ptr + 4 : NULL;

    if (body_ptr) *body_ptr = '\0'; 

    extract_request_line_fields(&request, request_line);
    extract_header_fields(&request, header_fields);
    extract_body(&request, body);
    
    free(requested);
    return request;
}

void extract_request_line_fields(struct HTTPRequest *request, char *request_line)
{
    if (!request_line) return;
    char *fields = strdup(request_line);
    char *saveptr;

    char *method = strtok_r(fields, " ", &saveptr);
    char *uri = strtok_r(NULL, " ", &saveptr);
    char *http_version = strtok_r(NULL, "\0", &saveptr);

    if (method) request->request_line.insert(&request->request_line, "method", 7, method, strlen(method) + 1);
    if (uri) request->request_line.insert(&request->request_line, "uri", 4, uri, strlen(uri) + 1);
    if (http_version) request->request_line.insert(&request->request_line, "http_version", 13, http_version, strlen(http_version) + 1);
    
    free(fields);
}

void extract_header_fields(struct HTTPRequest *request, char *header_fields)
{
    if (!header_fields) return;
    char *fields = strdup(header_fields);
    char *saveptr1, *saveptr2;
    
    char *line = strtok_r(fields, "\r\n", &saveptr1);
    while (line)
    {
        char *key = strtok_r(line, ":", &saveptr2);
        char *value = strtok_r(NULL, "\r\n", &saveptr2);
        if (key && value)
        {
            if (value[0] == ' ') value++;
            request->header_fields.insert(&request->header_fields, key, strlen(key) + 1, value, strlen(value) + 1);
        }
        line = strtok_r(NULL, "\r\n", &saveptr1);
    }
    free(fields);
}

void extract_body(struct HTTPRequest *request, char *body)
{
    if (!body) return;
    char *content_type = (char *)request->header_fields.search(&request->header_fields, "Content-Type", 13);
    
    if (content_type && strcmp(content_type, "application/x-www-form-urlencoded") == 0)
    {
        char *body_copy = strdup(body);
        char *saveptr1, *saveptr2;
        char *field = strtok_r(body_copy, "&", &saveptr1);
        while (field)
        {
            char *key = strtok_r(field, "=", &saveptr2);
            char *value = strtok_r(NULL, "\0", &saveptr2);
            if (key && value)
            {
                request->body.insert(&request->body, key, strlen(key) + 1, value, strlen(value) + 1);
            }
            field = strtok_r(NULL, "&", &saveptr1);
        }
        free(body_copy);
    }
    else if (body)
    {
        request->body.insert(&request->body, "data", 5, body, strlen(body) + 1);
    }
}
