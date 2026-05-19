#include "../../include/http_handler.h"
#include "../../include/network.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

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

void handle_filesystem_request(int client_fd, struct HTTPRequest* req) {
    char* method = (char*)req->request_line.search(&req->request_line, "method", sizeof("method"));
    char* uri = (char*)req->request_line.search(&req->request_line, "uri", sizeof("uri"));
    char* version = (char*)req->request_line.search(&req->request_line, "http_version", sizeof("http_version"));

    if (!method || !uri || !version) {
        send_error(client_fd, 400);
        return;
    }

    if (strcmp(version, "HTTP/1.1") != 0) {
        send_error(client_fd, 505);
        return;
    }

    if (strstr(uri, "../")) {
        send_error(client_fd, 403);
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
            send_error(client_fd, 411);
            return;
        }

        char* body_data = (char*)req->body.search(&req->body, "data", sizeof("data"));
        if (!body_data) body_data = "";

        printf("\n--- Received POST Payload data ---\n%s\n----------------------------------\n", body_data);

        mkdir("www", 0777); 
        int log_fd = open("www/post_data.txt", O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (log_fd != -1) {
            write(log_fd, body_data, strlen(body_data));
            write(log_fd, "\n", 1);
            close(log_fd);
        }

        send_created(client_fd, uri, keep_alive);
        return;
    }

    // GET and HEAD Method Execution Flow
    if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0) {
        send_error(client_fd, 405);
        return;
    }

    char local_path[512];
    snprintf(local_path, sizeof(local_path), "www%s", uri);
    if (local_path[strlen(local_path) - 1] == '/') {
        strncat(local_path, "index.html", sizeof(local_path) - strlen(local_path) - 1);
    }

    struct stat st;
    if (stat(local_path, &st) == -1 || S_ISDIR(st.st_mode)) {
        send_error(client_fd, 404);
        return;
    }

    int file_fd = open(local_path, O_RDONLY);
    if (file_fd == -1) {
        send_error(client_fd, 403);
        return;
    }

    const char* mime = get_mime_type(local_path);
    send_response_head(client_fd, "200", "OK", mime, st.st_size, NULL, keep_alive);

    if (strcmp(method, "GET") == 0) {
        send_file(client_fd, file_fd, 0, st.st_size);
    }

    close(file_fd);
}