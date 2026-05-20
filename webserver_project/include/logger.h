/* =========================================================
 * logger.h
 * ========================================================= */

#ifndef LOGGER_H
#define LOGGER_H

/**
 * Initialize logger subsystem.
 *
 * filepath == NULL:
 *      log to stdout
 *
 * filepath != NULL:
 *      append logs to file
 *
 * Returns:
 *      0  success
 *     -1  failure
 */
int logger_init(const char *filepath);

/**
 * Log one HTTP transaction.
 *
 * Format:
 * client_ip - - [timestamp] "METHOD PATH" status (Host: host)
 *
 * Example:
 * 127.0.0.1 - - [19/May/2026:20:15:11 +0200]
 * "GET /index.html" 200 (Host: localhost:8080)
 */
void logger_log_request(
    const char *client_ip,
    const char *method,
    const char *path,
    int status_code,
    const char *host
);

/**
 * Shutdown logger subsystem.
 */
void logger_destroy(void);

#endif