#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/network.h"
#include "include/thread_pool.h"
#include "include/http_handler.h"
#include "../DataStructures/Dictionary/Dictionary.h"

int main() {
    printf("Starting Let-us-Connect Server...\n");

    // 1. Prevent the server from crashing if a client disconnects unexpectedly
    setup_signals();

    // 2. Initialize the Dictionary to hold our route mappings
    struct Dictionary routes = dictionary_constructor(compare_string_keys);
    
    // FIX: Store the function pointers in variables first
    route_handler_t home_handler = handle_home;
    route_handler_t status_handler = handle_status;
    
    // Pass the ADDRESS of the variable so the dictionary stores the pointer correctly
    routes.insert(&routes, "/", sizeof("/"), &home_handler, sizeof(route_handler_t));
    routes.insert(&routes, "/api/status", sizeof("/api/status"), &status_handler, sizeof(route_handler_t));

    // 4. Initialize the Thread Pool
    thread_pool_t pool;
    if (thread_pool_init(&pool, 8, THREAD_POOL_SIZE_STATIC, "http_pool") != 0) {
        fprintf(stderr, "Failed to initialize thread pool.\n");
        return -1;
    }

    // 5. Initialize the Server Network on Port 8080
    int port = 8080;
    int server_fd = network_init(port);
    if (server_fd == -1) {
        fprintf(stderr, "Failed to initialize network.\n");
        return -1;
    }

    // 6. Run the server
    network_run_server(server_fd, &pool, &routes);

    // 7. Cleanup
    network_shutdown(server_fd);
    thread_pool_destroy(&pool, THREAD_POOL_DESTROY_HARD);
    dictionary_destructor(&routes);

    return 0;
}