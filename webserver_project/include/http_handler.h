#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include "http_parser.h"

void handle_filesystem_request(int client_fd, struct HTTPRequest* req);

#endif // HTTP_HANDLER_H