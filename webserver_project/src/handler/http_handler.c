#include "../../include/http_handler.h"
#include "../../include/network.h"
#include <string.h>
#include <stdio.h>

// Handler for the root route: "/"
void handle_home(int client_fd, struct HTTPRequest* req) {
    printf("Handling request for /\n");
    
    const char* body = 
        "<html>"
        "<head><title>Let-us-Connect</title></head>"
        "<body>"
        "<h1>Welcome to the Let-us-Connect Server!</h1>"
        "<p>Your C web server is successfully running.</p>"
        "</body>"
        "</html>";
        
    // Send the headers (200 OK, HTML content)
    send_response_head(client_fd, "200", "OK", "text/html", strlen(body), NULL, 0);
    
    // Send the actual HTML body
    send_all(client_fd, body, strlen(body));
}

// Handler for an API route: "/api/status"
void handle_status(int client_fd, struct HTTPRequest* req) {
    printf("Handling request for /api/status\n");
    
    const char* body = "{\"status\": \"online\", \"version\": \"1.0\"}";
    
    // Send the headers (200 OK, JSON content)
    send_response_head(client_fd, "200", "OK", "application/json", strlen(body), NULL, 0);
    
    // Send the actual JSON body
    send_all(client_fd, body, strlen(body));
}