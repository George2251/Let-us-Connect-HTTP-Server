// HTTPRequest.c
//
// weblibc
//

#include "HTTPRequest.h"

#include <string.h>
#include <stdio.h>

int method_select(char *method)
{
    if (method == NULL)
    {
        return -1;
    }

    if (strcmp(method, "GET") == 0)
    {
        return GET;
    }
    else if (strcmp(method, "POST") == 0)
    {
        return POST;
    }
    else if (strcmp(method, "PUT") == 0)
    {
        return PUT;
    }
    else if (strcmp(method, "HEAD") == 0)
    {
        return HEAD;
    }
    else if (strcmp(method, "DELETE") == 0)
    {
        return DELETE;
    }
    else if (strcmp(method, "CONNECT") == 0)
    {
        return CONNECT;
    }
    else if (strcmp(method, "OPTIONS") == 0)
    {
        return OPTIONS;
    }
    else if (strcmp(method, "PATCH") == 0)
    {
        return PATCH;
    }
    else if (strcmp(method, "TRACE") == 0)
    {
        return TRACE;
    }

    return -1;
}

struct HTTPRequest http_request_constructor(char *request_string, int buffer_length)
{
    // HTTP headers are separated from the body using \r\n\r\n
    // We also make sure we never read past the network buffer.

    int body_found = 0;
    struct HTTPRequest request;

    for (int i = 0; i < buffer_length - 3; i++)
    {
        if (request_string[i] == '\r' &&
            request_string[i + 1] == '\n' &&
            request_string[i + 2] == '\r' &&
            request_string[i + 3] == '\n')
        {
            request_string[i + 3] = '|';
            body_found = 1;
            break;
        }
    }

    // Prevent strtok from running off the end of the buffer
    // if the request is malformed or body separator is missing.

    if (!body_found)
    {
        request.Method = -1;
        request.URI = NULL;
        request.HTTPVersion = 0.0f;
        return request;
    }

    char *request_line = strtok(request_string, "\r\n");
    char *header_fields = strtok(NULL, "|");
    char *body = strtok(NULL, "|");

    char *method = strtok(request_line, " ");

    request.URI = strtok(NULL, " ");

    char *http_version = strtok(NULL, " ");

    if (http_version != NULL)
    {
        sscanf(http_version, "HTTP/%f", &request.HTTPVersion);
    }
    else
    {
        request.HTTPVersion = 0.0f;
    }

    request.Method = method_select(method);

    return request;
}
