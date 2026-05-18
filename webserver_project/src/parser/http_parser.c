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
    
    // FIX: Always initialize dictionaries so the destructor doesn't segfault on empty requests
    request.request_line = dictionary_constructor(compare_string_keys);
    request.header_fields = dictionary_constructor(compare_string_keys);
    request.body = dictionary_constructor(compare_string_keys);

    if (!request_string || strlen(request_string) == 0) return request;

    // FIX: Add +1 to array size for the null terminator (prevents Stack Smashing)
    char requested[strlen(request_string) + 1];
    strcpy(requested, request_string);

    // FIX: Properly handle browser \r\n line endings
    char* separator = strstr(requested, "\r\n\r\n");
    if (separator) {
        separator[0] = '|'; separator[1] = '|'; separator[2] = '|'; separator[3] = '|';
    } else {
        separator = strstr(requested, "\n\n");
        if (separator) { separator[0] = '|'; separator[1] = '|'; }
    }

    char *request_line = strtok(requested, "\r\n");
    char *header_fields = strtok(NULL, "|");
    char *body = strtok(NULL, "|");

    if (request_line) extract_request_line_fields(&request, request_line);
    if (header_fields) extract_header_fields(&request, header_fields);
    if (body) extract_body(&request, body);

    return request;
}

void http_request_destructor(struct HTTPRequest *request)
{
    dictionary_destructor(&request->request_line);
    dictionary_destructor(&request->header_fields);
    dictionary_destructor(&request->body);
}

void extract_request_line_fields(struct HTTPRequest *request, char *request_line)
{
    char fields[strlen(request_line) + 1];
    strcpy(fields, request_line);
    
    char *method = strtok(fields, " ");
    char *uri = strtok(NULL, " ");
    char *http_version = strtok(NULL, "\0");
    
    // FIX: Add +1 to value sizes to ensure strings in the dictionary are null-terminated
    if (method) request->request_line.insert(&request->request_line, "method", sizeof("method"), method, strlen(method) + 1);
    if (uri) request->request_line.insert(&request->request_line, "uri", sizeof("uri"), uri, strlen(uri) + 1);
    if (http_version) request->request_line.insert(&request->request_line, "http_version", sizeof("http_version"), http_version, strlen(http_version) + 1);
}

void extract_header_fields(struct HTTPRequest *request, char *header_fields)
{
    char fields[strlen(header_fields) + 1];
    strcpy(fields, header_fields);
    
    struct Queue headers = queue_constructor();
    char *field = strtok(fields, "\r\n");
    while (field)
    {
        headers.push(&headers, field, strlen(field) + 1);
        field = strtok(NULL, "\r\n");
    }
    
    char *header = (char *)headers.peek(&headers);
    while (header)
    {
        char *key = strtok(header, ":");
        char *value = strtok(NULL, "\0");
        if (value)
        {
            if (value[0] == ' ') value++;
            request->header_fields.insert(&request->header_fields, key, strlen(key) + 1, value, strlen(value) + 1);
        }
        headers.pop(&headers);
        header = (char *)headers.peek(&headers);
    }
    queue_destructor(&headers);
}

void extract_body(struct HTTPRequest *request, char *body)
{
    char *content_type = (char *)request->header_fields.search(&request->header_fields, "Content-Type", sizeof("Content-Type"));
    if (content_type)
    {
        if (strcmp(content_type, "application/x-www-form-urlencoded") == 0)
        {
            struct Queue fields = queue_constructor();
            char *field = strtok(body, "&");
            while (field)
            {
                fields.push(&fields, field, strlen(field) + 1);
                field = strtok(NULL, "&");
            }
            
            field = fields.peek(&fields);
            while (field)
            {
                char *key = strtok(field, "=");
                char *value = strtok(NULL, "\0");
                if (value && value[0] == ' ') value++;
                if (key && value) request->body.insert(&request->body, key, strlen(key) + 1, value, strlen(value) + 1);
                
                fields.pop(&fields);
                field = fields.peek(&fields);
            }
            queue_destructor(&fields);
        }
        else
        {
            request->body.insert(&request->body, "data", sizeof("data"), body, strlen(body) + 1);
        }
    }
}