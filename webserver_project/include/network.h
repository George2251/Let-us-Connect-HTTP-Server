#ifndef NETWORK_H
#define NETWORK_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "http_parser.h"
struct Dictionary; // Keep this one as a forward declaration

// Define what a route handler function looks like
typedef void (*route_handler_t)(int client_fd, struct HTTPRequest* req);//defines what a route handler function looks like

// Define the task structure that gets passed to the worker threads
struct ClientTask {
    int client_fd;
    struct Dictionary* routes; 
};

// Include the thread pool header provided by the Thread Pool team
#include "thread_pool.h"


int network_init(int port); // port: listen port, returns server socket fd or -1 on error - implemented
void network_run_server(int server_fd, thread_pool_t* pl, struct Dictionary* routes); // server_fd: listening socket, pl: thread pool, routes: route handlers - implemented
void network_shutdown(int server_fd); // server_fd: socket to close - implemented

int send_all(int fd, const char* buf, int len); // fd: socket, buf/len: data to send, returns 0 or -1 - implemented
int send_file(int fd, int file_fd, off_t offset, int len); // fd: socket, file_fd: open file, offset/len: range, returns 0 or -1 - implemented
int send_response_head(int fd, const char* status, const char* text, const char* ct, int cl, const char* extra, int ka); // fd: socket, status/text/ct: headers, cl: length, extra: extra headers, ka: keep-alive flag - implemented
void send_error(int fd, int code); // fd: socket, code: HTTP error status
void send_not_modified(int fd, const char* last_modified, int ka); // fd: socket, last_modified: timestamp string, ka: keep-alive flag - implemented
void send_created(int fd, const char* location, int ka); // fd: socket, location: resource URL, ka: keep-alive flag

int make_nonblocking(int sockfd); // sockfd: socket to set non-blocking, returns 0 or -1 The command constant that tells fcntl to "Get File status Flags".
void setup_signals(void); // installs OS signal handlers
void check_keepalive_timeouts(void); // checks and closes timed-out keep-alive connections

#endif // NETWORK_H