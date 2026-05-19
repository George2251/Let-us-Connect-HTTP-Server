#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "include/network.h"
#include "include/thread_pool.h"
#include "include/http_handler.h"
#include "../DataStructures/Dictionary/Dictionary.h"

int main() {
    // 1. Structural setup: Make sure our targeted filesystem root directory exists
    mkdir("www", 0777);
    
    // Create a generic home file if none exists to facilitate immediate evaluation
    FILE* f = fopen("www/index.html", "r");
    if (!f) {
        f = fopen("www/index.html", "w");
        if (f) {
            fprintf(f, "<html><body><h1>Let-us-Connect Functional Ecosystem</h1><p>Non-blocking Keep-Alive Operational.</p></body></html>");
            fclose(f);
        }
    } else {
        fclose(f);
    }

    printf("Starting Let-us-Connect Server Configuration Pipeline...\n");
    setup_signals();

    struct Dictionary routes = dictionary_constructor(compare_string_keys);
    
    // Map wildcard symbol token to default filesystem core handler 
    route_handler_t fs_router = handle_filesystem_request;
    routes.insert(&routes, "*", sizeof("*"), &fs_router, sizeof(route_handler_t));

    thread_pool_t pool;
    if (thread_pool_init(&pool, 8, THREAD_POOL_SIZE_STATIC, "http_pool") != 0) {
        fprintf(stderr, "Fatal configuration failure: queue thread engine initialization halted.\n");
        return -1;
    }

    int port = 8080;
    int server_fd = network_init(port);
    if (server_fd == -1) return -1;

    network_run_server(server_fd, &pool, &routes);

    network_shutdown(server_fd);
    thread_pool_destroy(&pool, THREAD_POOL_DESTROY_HARD);
    dictionary_destructor(&routes);

    return 0;
}