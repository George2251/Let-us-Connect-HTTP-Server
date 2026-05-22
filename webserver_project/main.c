/* =========================================================
 * main.c
 * ========================================================= */

#include "../include/network.h"
#include "../include/http_handler.h"
#include "../include/logger.h"
#include "../include/thread_pool.h"

#include "../../DataStructures/Dictionary/Dictionary.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * Route wrapper
 */
static void filesystem_route(
    int client_fd,
    struct HTTPRequest *req)
{
    handle_filesystem_request(client_fd, req);
}

int main(int argc, char *argv[])
{
    /* =====================================================
     * Configurations
     * ===================================================== */

    int port = 8080;

    int num_of_threads = 8;

    /*
     * Example:
     * ./server 16
     */
    if (argc > 1)
    {
        num_of_threads = atoi(argv[1]);

        if (num_of_threads <= 0)
        {
            num_of_threads = 8;
        }
    }

    printf(
        "Starting server with %d worker threads...\n",
        num_of_threads);

    /* =====================================================
     * Setup signals
     * ===================================================== */

    setup_signals();

    /* =====================================================
     * Initialize logger
     * ===================================================== */

    if (logger_init("server.log") != 0)
    {
        fprintf(stderr,
                "Failed to initialize logger\n");

        return 1;
    }

    /* =====================================================
     * Initialize thread pool
     * ===================================================== */

    thread_pool_t pool;

    if (thread_pool_init(
            &pool,
            num_of_threads,
            THREAD_POOL_SIZE_STATIC,
            "worker_pool") != 0)
    {
        fprintf(stderr,
                "Failed to initialize thread pool\n");

        logger_destroy();

        return 1;
    }

    /* =====================================================
     * Create routing table
     * ===================================================== */

    struct Dictionary routes =
        dictionary_constructor(compare_string_keys);

    /*
     * Register default filesystem handler
     *
     * "*" means:
     * any route not explicitly found
     */
    route_handler_t fs_handler =
        filesystem_route;

    routes.insert(
        &routes,
        "*",
        sizeof("*"),
        &fs_handler,
        sizeof(route_handler_t));

    /* =====================================================
     * Initialize server socket
     * ===================================================== */

    int server_fd = network_init(port);

    if (server_fd == -1)
    {
        fprintf(stderr,
                "Failed to initialize network\n");

        thread_pool_destroy(
            &pool,
            THREAD_POOL_DESTROY_SOFT);

        dictionary_destructor(&routes);

        logger_destroy();

        return 1;
    }

    printf(
        "Server listening on port %d...\n",
        port);

    /* =====================================================
     * Main server loop
     * ===================================================== */

    network_run_server(
        server_fd,
        &pool,
        &routes);

    /* =====================================================
     * Cleanup
     * ===================================================== */

    network_shutdown(server_fd);

    thread_pool_destroy(
        &pool,
        THREAD_POOL_DESTROY_SOFT);

    dictionary_destructor(&routes);

    logger_destroy();

    return 0;
}