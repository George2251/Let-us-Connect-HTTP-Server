#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include "http_parser.h"

// Example Route Handlers
void handle_home(int client_fd, struct HTTPRequest* req);
void handle_status(int client_fd, struct HTTPRequest* req);

#endif // HTTP_HANDLER_H