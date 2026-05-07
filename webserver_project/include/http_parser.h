// HTTPRequest.h
//
// weblibc
//


#ifndef HTTPRequest_h
#define HTTPRequest_h

enum HTTPMethods
{
    GET,
    POST,
    PUT,
    HEAD,
    PATCH,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE
};

struct HTTPRequest
{
    int Method;
    char *URI;
    float HTTPVersion;
};

struct HTTPRequest http_request_constructor(char *request_string, int buffer_length);

#endif /* HTTPRequest_h */
