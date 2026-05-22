#include "../../include/http_handler.h"
#include "../../include/network.h"
#include "../include/logger.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

static const char* get_mime_type(const char* path) {
    const char* ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (strcasecmp(ext, ".html") == 0 || strcasecmp(ext, ".htm") == 0) return "text/html";
    if (strcasecmp(ext, ".css") == 0) return "text/css";
    if (strcasecmp(ext, ".js") == 0) return "application/javascript";
    if (strcasecmp(ext, ".png") == 0) return "image/png";
    if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0) return "image/jpeg";
    return "application/octet-stream";
}

/*
 * Extract client IP from socket fd
 */
static void get_client_ip(
    int client_fd,
    char *buffer,
    size_t size)
{
    struct sockaddr_in addr;

    socklen_t addr_len = sizeof(addr);

    if (getpeername(
            client_fd,
            (struct sockaddr *)&addr,
            &addr_len) == -1)
    {
        strncpy(buffer, "-", size);
        buffer[size - 1] = '\0';
        return;
    }

    inet_ntop(
        AF_INET,
        &addr.sin_addr,
        buffer,
        size);
}

void handle_filesystem_request(int client_fd, struct HTTPRequest* req) {

       /*
     * Build client IP for logger
     */
    char client_ip[INET_ADDRSTRLEN];

get_client_ip(
    client_fd,
    client_ip,
    sizeof(client_ip));
    char* method = (char*)req->request_line.search(&req->request_line, "method", sizeof("method"));
    char* uri = (char*)req->request_line.search(&req->request_line, "uri", sizeof("uri"));
    char* version = (char*)req->request_line.search(&req->request_line, "http_version", sizeof("http_version"));
     /*
     * Extract Host header
     */
    char* host =
        (char*)req->header_fields.search(
            &req->header_fields,
            "host",
            sizeof("host"));

    /*
    If not worked Try this
    char *host = req->header_fields.search(
    &req->header_fields,
    "Host",
    strlen("Host") + 1);
                  
    */

    if (!method || !uri || !version) {
        send_error(client_fd, 400);// Bad Request
        logger_log_request(
            client_ip,
            method,
            uri,
            400,
            host);
        return;
    }

    if (strcmp(version, "HTTP/1.1") != 0) {
        send_error(client_fd, 505); // HTTP Version Not Supported
        logger_log_request(
            client_ip,
            method,
            uri,
            505,
            host);
        return;
    }

    if (strstr(uri, "../")) {
        send_error(client_fd, 403);// Forbidden
        logger_log_request(
            client_ip,
            method,
            uri,
            403,
            host);
        return;
    }

    // FIX: Search lowercase key and enforce HTTP/1.1 Keep-Alive defaults
    char* connection_val = (char*)req->header_fields.search(&req->header_fields, "connection", sizeof("connection"));
    int keep_alive = 1; // Default to TRUE
    if (connection_val && strcasecmp(connection_val, "close") == 0) keep_alive = 0;

    // POST Method Execution Flow
    if (strcmp(method, "POST") == 0) {
        // FIX: Search lowercase key
        char* cl_str = (char*)req->header_fields.search(&req->header_fields, "content-length", sizeof("content-length"));
        if (!cl_str) {
            send_error(client_fd, 411);// Length Required
             logger_log_request(
                client_ip,
                method,
                uri,
                411,
                host);
            return;
        }

        char* body_data = (char*)req->body.search(&req->body, "data", sizeof("data"));
        if (!body_data) body_data = "";

        printf("\n--- Received POST Payload data ---\n%s\n----------------------------------\n", body_data);

        mkdir("public", 0777); 
        int log_fd = open("public/post_data.txt", O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (log_fd != -1) {
            write(log_fd, body_data, strlen(body_data));
            write(log_fd, "\n", 1);
            close(log_fd);
        }

        send_created(client_fd, uri, keep_alive);
        logger_log_request(
            client_ip,
            method,
            uri,
            201,
            host);
        return;
    }

    // GET and HEAD Method Execution Flow
    if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0) {
        send_error(client_fd, 405);
         logger_log_request(
            client_ip,
            method,
            uri,
            405,
            host);
        return;
    }

    char local_path[512];
    snprintf(local_path, sizeof(local_path), "public%s", uri);

    struct stat st;
    
    // 1. Try to find the exact file or directory first
    if (stat(local_path, &st) == 0) {
        
        // 2. If it is a directory, automatically look for index.html inside it
        if (S_ISDIR(st.st_mode)) {
            if (local_path[strlen(local_path) - 1] != '/') {
                strncat(local_path, "/", sizeof(local_path) - strlen(local_path) - 1);
            }
            strncat(local_path, "index.html", sizeof(local_path) - strlen(local_path) - 1);
            
            // Re-check if the index.html actually exists
            if (stat(local_path, &st) == -1 || S_ISDIR(st.st_mode)) {
                send_error(client_fd, 404); // Not Found
                logger_log_request(
                    client_ip,
                    method,
                    uri,
                    404,
                    host);
                return;
            }
        }
    } else {
        // 3. The exact file wasn't found. Try appending ".html" to the path
        // This allows extensionless URLs (e.g., "/about" -> "/about.html")
        strncat(local_path, ".html", sizeof(local_path) - strlen(local_path) - 1);
        
        if (stat(local_path, &st) == -1 || S_ISDIR(st.st_mode)) {
            send_error(client_fd, 404); // Not Found
            logger_log_request(
                client_ip,
                method,
                uri,
                404,
                host);
            return;
        }
    }

    // 4. File is found and confirmed to be a standard file. Open and send it!
    int file_fd = open(local_path, O_RDONLY);
    if (file_fd == -1) {
        send_error(client_fd, 403); // Forbidden
         logger_log_request(
            client_ip,
            method,
            uri,
            403,
            host);
        return;
    }

    const char* mime = get_mime_type(local_path);
    send_response_head(client_fd, "200", "OK", mime, st.st_size, NULL, keep_alive);

    if (strcmp(method, "GET") == 0) {
        send_file(client_fd, file_fd, 0, st.st_size);
    }

    close(file_fd);
    //Log successful transaction

    logger_log_request(
        client_ip,
        method,
        uri,
        200,
        host);
}