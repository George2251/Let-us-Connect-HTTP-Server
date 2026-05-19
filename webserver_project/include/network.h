#ifndef NETWORK_H
#define NETWORK_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

struct HTTPRequest;
struct Dictionary;

// Tracks active connections for Keep-Alive multiplexing
struct ClientConnection {
    int fd;
    time_t last_activity;
    int is_busy;   // 1 if currently processing inside the thread pool
    int is_active; // 1 if slot contains a valid open connection
};

typedef void (*route_handler_t)(int client_fd, struct HTTPRequest* req);

struct ClientTask {
    int client_fd;
    struct Dictionary* routes; 
    struct ClientConnection* conn; // Links task to its connection tracker
};

#include "thread_pool.h"

int network_init(int port); 
void network_run_server(int server_fd, thread_pool_t* pl, struct Dictionary* routes); 
void network_shutdown(int server_fd); 

int send_all(int fd, const char* buf, int len); 
int send_file(int fd, int file_fd, off_t offset, int len); 
int send_response_head(int fd, const char* status, const char* text, const char* ct, int cl, const char* extra, int ka); 
void send_error(int fd, int code); 
void send_not_modified(int fd, const char* last_modified, int ka); 
void send_created(int fd, const char* location, int ka); 

int make_nonblocking(int sockfd); 
void setup_signals(void); 
void check_keepalive_timeouts(void); 

#endif // NETWORK_H