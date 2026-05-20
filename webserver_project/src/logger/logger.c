/* =========================================================
 * logger.c
 * ========================================================= */

#include "../include/logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/* =========================================================
 * Internal State
 * ========================================================= */

static FILE *log_stream = NULL;

static int logger_initialized = 0;

/*
 * Protects:
 * - initialization
 * - writes
 * - destruction
 */
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/* =========================================================
 * Internal Helpers
 * ========================================================= */

/*
 * Timestamp format:
 * 19/May/2026:20:15:11 +0200
 */
static void format_timestamp(char *buffer, size_t size)
{
    time_t now = time(NULL);

    struct tm tm_info;

    localtime_r(&now, &tm_info);

    strftime(buffer,
             size,
             "%d/%b/%Y:%H:%M:%S %z",
             &tm_info);
}

/* =========================================================
 * Public API
 * ========================================================= */

int logger_init(const char *filepath)
{
    pthread_mutex_lock(&log_mutex);

    /*
     * Prevent double initialization
     */
    if (logger_initialized)
    {
        pthread_mutex_unlock(&log_mutex);
        return 0;
    }

    /*
     * stdout logging
     */
    if (filepath == NULL)
    {
        log_stream = stdout;
    }
    else
    {
        /*
         * Append mode preserves old logs
         */
        log_stream = fopen(filepath, "a");

        if (log_stream == NULL)
        {
            pthread_mutex_unlock(&log_mutex);
            return -1;
        }
    }

    logger_initialized = 1;

    pthread_mutex_unlock(&log_mutex);

    return 0;
}

void logger_log_request(
    const char *client_ip,
    const char *method,
    const char *path,
    int status_code,
    const char *host)
{
    pthread_mutex_lock(&log_mutex);

    /*
     * Prevent race with destroy()
     */
    if (!logger_initialized || log_stream == NULL)
    {
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    /*
     * Default fallback values
     */
    if (client_ip == NULL)
        client_ip = "-";

    if (method == NULL)
        method = "-";

    if (path == NULL)
        path = "-";

    if (host == NULL)
        host = "-";

    /*
     * Build timestamp
     */
    char timestamp[64];

    format_timestamp(timestamp, sizeof(timestamp));

    /*
     * Write complete log line atomically
     */
    fprintf(
        log_stream,
        "%s - - [%s] \"%s %s\" %d (Host: %s)\n",
        client_ip,
        timestamp,
        method,
        path,
        status_code,
        host);

    /*
     * Force immediate write
     */
    fflush(log_stream);

    pthread_mutex_unlock(&log_mutex);
}

void logger_destroy(void)
{
    pthread_mutex_lock(&log_mutex);

    if (!logger_initialized)
    {
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    /*
     * Close file if not stdout
     */
    if (log_stream != stdout)
    {
        fclose(log_stream);
    }

    log_stream = NULL;

    logger_initialized = 0;

    pthread_mutex_unlock(&log_mutex);

    /*
     * Do NOT destroy statically initialized mutex
     * in long-running servers.
     */
}