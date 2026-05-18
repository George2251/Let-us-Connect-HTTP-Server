#include "../../include/network.h"
#include "../../include/http_parser.h" // For struct Dictionary definition (if needed by ClientTask)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>       // For close()
#include <fcntl.h>        // For fcntl(), F_GETFL, F_SETFL, O_NONBLOCK
#include <signal.h>       // For signal(), SIGPIPE, SIG_IGN
#include <sys/socket.h>
#include <sys/select.h>   // For select(), fd_set
#include <sys/sendfile.h> // For zero-copy send_file()
#include <arpa/inet.h>    // For inet_ntoa, htons, htonl

/* =========================================================
 * 1. Core Networking & Server Lifecycle (Main Thread)
 * ========================================================= */

int network_init(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    int opt = 1;

    if (server_fd == -1) {
        perror("socket creation failed");
        return -1; 
    }

    // Allow immediate port reuse after server restart
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt SO_REUSEADDR failed");
        close(server_fd);
        return -1;
    }

    if(make_nonblocking(server_fd) == -1) {
        perror("failed to set non-blocking");
        close(server_fd);
        return -1; 
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to all available network interfaces
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("Listen failed");
        close(server_fd);
        return -1;
    }

    printf("Server successfully initialized and listening on port %d\n", port);
    return server_fd;
}

// MODIFIED: Now takes the 'routes' dictionary to pass down to the workers
void network_run_server(int server_fd, thread_pool_t* pl, struct Dictionary* routes) {
    fd_set read_fds;
    int max_fd = server_fd;

    printf("Entering main server loop...\n");
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        
        if (activity < 0 && errno != EINTR) {
            perror("select error");
            break;
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd;

            // Loop to drain all pending connections (since socket is non-blocking)
            while ((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len)) != -1) {
                
                printf("New connection accepted from %s:%d (fd: %d)\n",
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_fd);

                // Make the client socket non-blocking
                //make_nonblocking(client_fd); resulted in race condition where worker thread tries to read before this is set, causing EAGAIN errors. Will set in worker thread instead.

                // MODIFIED: Create the ClientTask struct...
                struct ClientTask *new_task = malloc(sizeof(struct ClientTask));
                if (!new_task) {
                    perror("Failed to allocate memory for new client task");
                    close(client_fd);
                    continue;
                }
                
                new_task->client_fd = client_fd;
                new_task->routes = routes;

                // DELEGATE: Pass the task struct to the thread pool
                if (add_work(pl, new_task) != 0) {
                    fprintf(stderr, "Thread pool queue full. Dropping client %d\n", client_fd);
                    close(client_fd);
                    free(new_task); // Clean up memory if queue is full
                }
            }

            // After draining, accept returns -1 and sets errno to EAGAIN/EWOULDBLOCK
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("Accept failed");
            }
        }
        check_keepalive_timeouts();
    }
}

void network_shutdown(int server_fd) {
    printf("\nShutting down server...\n");
    close(server_fd);
}

/* =========================================================
 * 2. Response Utilities (Used by Worker Threads)
 * ========================================================= */

int send_all(int fd, const char* buf, int len) {
    int total = 0;        // How many bytes we've sent
    int bytesleft = len;  // How many we have left to send
    int n;

    while (total < len) {
        n = send(fd, buf + total, bytesleft, 0);
        
        if (n == -1) {
            if (errno == EINTR) continue; 
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue; 
            break; // Hard error
        }
        total += n;
        bytesleft -= n;
    }

    return n == -1 ? -1 : 0; 
}

int send_file(int fd, int file_fd, off_t offset, int len) {
    int total = 0;
    int bytesleft = len;
    ssize_t n;

    while (total < len) {
        n = sendfile(fd, file_fd, &offset, bytesleft);
        
        if (n == -1) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            perror("sendfile failed");
            break;
        }
        if (n == 0) break; // EOF reached

        total += n;
        bytesleft -= n;
    }
    
    return n == -1 ? -1 : 0;
}

int send_response_head(int fd, const char* status, const char* text, const char* ct, int cl, const char* extra, int ka) {
    char header_buffer[1024];
    
    int len = snprintf(header_buffer, sizeof(header_buffer),
        "HTTP/1.1 %s %s\r\n"
        "Server: Let-us-Connect/1.0\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: %s\r\n"
        "%s" 
        "\r\n", 
        status, text, ct, cl, (ka ? "keep-alive" : "close"), (extra ? extra : ""));
        
    return send_all(fd, header_buffer, len);
}

// RESTORED: send_error was missing from your snippet
void send_error(int fd, int code) {
    char body[512];
    char status_str[10];
    char* text;

    switch (code) {
        case 404: text = "Not Found"; break;
        case 400: text = "Bad Request"; break;
        case 403: text = "Forbidden"; break;
        case 405: text = "Method Not Allowed"; break;
        case 501: text = "Not Implemented"; break;
        default: text = "Internal Server Error"; break;
    }

    sprintf(status_str, "%d", code);
    int body_len = snprintf(body, sizeof(body),
        "<html><body><h1>%d %s</h1><p>Let-us-Connect Server</p></body></html>", code, text);

    send_response_head(fd, status_str, text, "text/html", body_len, "", 0);
    send_all(fd, body, body_len);
}

void send_not_modified(int fd, const char* last_modified, int ka) {
    char extra_headers[256];
    snprintf(extra_headers, sizeof(extra_headers), "Last-Modified: %s\r\n", last_modified);
    
    send_response_head(fd, "304", "Not Modified", "text/plain", 0, extra_headers, ka);
}

// RESTORED: send_created was missing from your snippet
void send_created(int fd, const char* location, int ka) {
    char extra_headers[256];
    snprintf(extra_headers, sizeof(extra_headers), "Location: %s\r\n", location);
    
    char* body = "Resource successfully created.\n";
    send_response_head(fd, "201", "Created", "text/plain", strlen(body), extra_headers, ka);
    send_all(fd, body, strlen(body));
}

/* =========================================================
 * 3. Internal Helpers & OS Assurances
 * ========================================================= */

int make_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        return -1;
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK failed");
        return -1;
    }

    return 0;
}

// RESTORED: Important for preventing server crashes on broken pipes
void setup_signals(void) {
    // Ignore SIGPIPE so the server doesn't crash if a client closes connection during send()
    signal(SIGPIPE, SIG_IGN);
}

void check_keepalive_timeouts(void) {
    // Background sweep logic for keeping track of active connections
    // Currently a placeholder to be filled out if Keep-Alive state tracking is implemented.
}