#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "../../include/thread_pool.h"
#include "../../include/network.h"
#include "../../include/http_parser.h"
#include "../../DataStructures/Dictionary/Dictionary.h"

int thread_pool_init(thread_pool_t *thrd_pl, int max_thread_num, int size_option, const char *pool_name)
{
    // check for invalid input
    if ((max_thread_num < 1) || (pool_name == NULL) || ((size_option != THREAD_POOL_SIZE_STATIC) && (size_option != THREAD_POOL_SIZE_DYNAMIC)))
        return INVALID_INPUT;

    thrd_pl->max_size = max_thread_num;

    // check whether to choose the curr_size based on the size_option
    // static size means that the size is constant
    // dynamic size means that the size changes based on demand (NOT implemented yet)
    if ((thrd_pl->size_option = size_option) == THREAD_POOL_SIZE_DYNAMIC)
    {
        thrd_pl->curr_size = (max_thread_num < INITIAL_POOL_SIZE) ? max_thread_num : INITIAL_POOL_SIZE;
    }
    else
    {
        thrd_pl->curr_size = max_thread_num;
    }

    // set the task-related members to reset values
    thrd_pl->available_work = 0;
    thrd_pl->busy_threads_num = 0;

    // the pool name is used for the semaphore
    thrd_pl->pool_name = pool_name;

    // initialize the queue that would carry the tasks
    if (fifo_queue_init(&thrd_pl->queue) != 0)
    {
        return ERROR_INITIALIZING_QUEUE;
    }

    // initialize the lock that would be used to protect the data structure
    // from race conditions while editing it
    thrd_pl->pool_lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (pthread_mutex_init(thrd_pl->pool_lock, NULL) != 0)
    {
        return ERRNO_ERROR;
    }

    // initialize the semaphore which will wake threads when ever ther is work
    if ((thrd_pl->pool_sem = sem_open(thrd_pl->pool_name, O_CREAT, 0666, 0)) == SEM_FAILED)
    {
        pthread_mutex_destroy(thrd_pl->pool_lock);
        return ERRNO_ERROR;
    }

    // initialize the thread info related arrays
    thrd_pl->threads_states = (thread_state **)malloc(thrd_pl->curr_size * sizeof(thread_state *));
    thrd_pl->tids = (pthread_t *)malloc(thrd_pl->curr_size * sizeof(pthread_t));

    // initialize the thread attributes
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    // threads arguments, which will be packaged and sent to the thread
    thread_args *args = (thread_args *)malloc(sizeof(thread_args));
    args->thrd_pl = thrd_pl;

    // this semaphore will be used only to initialize each thread
    sem_t *pool_init_sem = sem_open("INIT_SEM", O_CREAT, 0666, 0);
    args->init_sem = pool_init_sem; // just for initializing the pool
    for (int i = 0; i < thrd_pl->curr_size; i++)
    {

        // give each thread a pointer to its state information so
        // the both the main thread and child thread can read and write the state
        thrd_pl->threads_states[i] = (thread_state *)malloc(sizeof(thread_state));
        *(thrd_pl->threads_states[i]) = FREE;
        args->state = thrd_pl->threads_states[i];

        pthread_create(&thrd_pl->tids[i], &attr, _thread_task, (void *)args);

        // wait for the just created thread to complete its initialization
        sem_wait(pool_init_sem);
    }

    // destroy those since they are only for initialization
    sem_unlink("INIT_SEM");
    free(args);

    return 0;
}

void *_thread_task(void *args)
{
    // give the thread pointers to its state and thread pool
    thread_state *state = ((thread_args *)args)->state;
    thread_pool_t *thrd_pl = ((thread_args *)args)->thrd_pl;

    // let the thread pool continue initializing the other threads
    sem_post(((thread_args *)args)->init_sem);

    while (1)
    {
        // wait here untill a work is available
        sem_wait(thrd_pl->pool_sem);

        // lock the thread pool so no race conditions happen
        pthread_mutex_lock(thrd_pl->pool_lock);

        // this struct will carry the client socket and the routes dictionary,
        // it is allocated in network.c when a new client connects and is added
        // to the queue as work for the thread pool, here we pop it from the queue
        // and use its data to handle the request
        struct ClientTask *task = NULL;

        // check if the main thread demands that this thread
        // to be closed
        if (*state == TERMINATED)
        {
            pthread_mutex_unlock(thrd_pl->pool_lock);
            break;
        }

        if (thrd_pl->available_work > 0)
        { // if there is work to do

            // get the work and edit the thread pool work-related data members
            pop_first(&thrd_pl->queue, &task);
            thrd_pl->available_work--;
            thrd_pl->busy_threads_num++;
            *state = BUSY;
            pthread_mutex_unlock(thrd_pl->pool_lock);
        }
        else
        { // if there is not work, this can be if a signal incorrectly happened
            pthread_mutex_unlock(thrd_pl->pool_lock);
            continue;
        }

        // do the actuall work here
        //*******************************************************************/
        if (task != NULL) {
            int client_fd = task->client_fd;
            struct ClientConnection* conn = task->conn;
            
            // 1. Read available data into the client's personal persistent buffer
            ssize_t bytes_read = recv(client_fd, conn->read_buffer + conn->read_length, 
                                      sizeof(conn->read_buffer) - 1 - conn->read_length, 0);

            if (bytes_read > 0) {
                conn->read_length += bytes_read;
                conn->read_buffer[conn->read_length] = '\0'; // Ensure string termination
                
                // 2. STATE MACHINE: Is the HTTP request completely finished?
                int request_complete = 0;
                char* headers_end = strstr(conn->read_buffer, "\r\n\r\n");
                
                if (headers_end) {
                    // Headers are finished. Do we need to wait for a POST body?
                    char* cl_str = strstr(conn->read_buffer, "Content-Length:");
                    if (cl_str) {
                        int content_length = atoi(cl_str + 15); // Skip "Content-Length:"
                        int headers_length = (headers_end - conn->read_buffer) + 4; // +4 for \r\n\r\n
                        
                        // Have we received the headers PLUS the entire body payload?
                        if (conn->read_length >= headers_length + content_length) {
                            request_complete = 1;
                        }
                    } else {
                        // No Content-Length (like a standard GET), so request is done!
                        request_complete = 1; 
                    }
                }

                // 3. Process ONLY if the payload is fully assembled
                if (request_complete) {
                    struct HTTPRequest req = http_request_constructor(conn->read_buffer);
                    
                    char* connection_val = (char*)req.header_fields.search(&req.header_fields, "Connection", sizeof("Connection"));
                    int keep_alive = 1; // Default HTTP/1.1
                    if (connection_val && strcasecmp(connection_val, "close") == 0) keep_alive = 0; 

                    char* uri = (char*)req.request_line.search(&req.request_line, "uri", sizeof("uri"));
                    route_handler_t *handler_ptr = NULL;

                    if (uri != NULL && task->routes != NULL) {
                        handler_ptr = (route_handler_t*)task->routes->search(task->routes, uri, strlen(uri) + 1);
                    }

                    if (handler_ptr != NULL) {
                        (*handler_ptr)(client_fd, &req);
                    } else {
                        route_handler_t *fs_handler_ptr = (route_handler_t *)task->routes->search(task->routes, "*", sizeof("*"));
                        if (fs_handler_ptr != NULL) {
                            (*fs_handler_ptr)(client_fd, &req);
                        } else {
                            send_error(client_fd, 404);
                        }
                    }

                    http_request_destructor(&req);

                    // 4. Reset the persistent buffer for the next Keep-Alive request
                    conn->read_length = 0;
                    memset(conn->read_buffer, 0, sizeof(conn->read_buffer));

                    if (keep_alive && conn->is_active) {
                        conn->last_activity = time(NULL);
                        conn->is_busy = 0; // Hand back to select loop
                    } else {
                        close(client_fd);
                        conn->is_active = 0;
                    }
                } else {
                    // 5. FRAGMENTED REQUEST: Not enough data yet. Yield thread back to pool!
                    conn->last_activity = time(NULL);
                    conn->is_busy = 0;
                }
                
            } else if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                // 6. SPECULATIVE CONNECTION: No data sent yet. Yield thread back to pool!
                conn->is_busy = 0; 
            } else {
                // 7. CLIENT DISCONNECT: Browser closed the tab or threw a network error
                close(client_fd);
                conn->is_active = 0;
            }
            free(task);
        }
        //*******************************************************************/
        // end of work

        // lock the thread pool again to reflect that this thread
        // is no longer busy doing work
        pthread_mutex_lock(thrd_pl->pool_lock);

        // check if the main thread demands that this thread
        // to be closed, the reason that this is repeated that the state
        // would be written by the thread later
        if (*state == TERMINATED)
        {
            pthread_mutex_unlock(thrd_pl->pool_lock);
            break;
        }

        thrd_pl->busy_threads_num--;
        *state = FREE;

        pthread_mutex_unlock(thrd_pl->pool_lock);
    }

    pthread_exit(0);
}

int add_work(thread_pool_t *thrd_pl, struct ClientTask *task)
{
    // check for the input
    if (thrd_pl == NULL)
        return INVALID_INPUT;

    // choose how to add work based on the size_option
    if (thrd_pl->size_option == THREAD_POOL_SIZE_STATIC)
    { // size is static, just add the work
        pthread_mutex_lock(thrd_pl->pool_lock);

        push_last(&thrd_pl->queue, task);
        thrd_pl->available_work++;
        sem_post(thrd_pl->pool_sem);

        pthread_mutex_unlock(thrd_pl->pool_lock);
    }
    else
    { // size is dynamic, check if resizing is needed first
    }

    return 0;
}

int thread_pool_destroy(thread_pool_t *thrd_pl, int option)
{

    // check for inputs
    if ((thrd_pl == NULL) || ((option != THREAD_POOL_DESTROY_SOFT) &&
                              (option != THREAD_POOL_DESTROY_DRAIN_AND_DROP) && (option != THREAD_POOL_DESTROY_HARD)))
        return INVALID_INPUT;

    if ((option == THREAD_POOL_DESTROY_SOFT) || (option == THREAD_POOL_DESTROY_DRAIN_AND_DROP))
    { // wait till every thread has finished its work

        // wait untill all the queued work is comleted
        if (option == THREAD_POOL_DESTROY_SOFT)
        {
            while (thrd_pl->available_work > 0)
            {
            }
        }

        // tell every thread that it is terminated and should exit
        pthread_mutex_lock(thrd_pl->pool_lock);
        for (int i = 0; i < thrd_pl->curr_size; i++)
        {
            *(thrd_pl->threads_states[i]) = TERMINATED;
        }
        pthread_mutex_unlock(thrd_pl->pool_lock);

        // signal for every thread, more signals isn't bad
        for (int i = 0; i < thrd_pl->curr_size; i++)
        {
            sem_post(thrd_pl->pool_sem);
        }
    }
    else
    { // kill every thread immediately
        for (int i = 0; i < thrd_pl->curr_size; i++)
            pthread_cancel(thrd_pl->tids[i]);
    }

    // join on all of them
    for (int i = 0; i < thrd_pl->curr_size; i++)
    {
        pthread_join(thrd_pl->tids[i], NULL);
        free(thrd_pl->threads_states[i]);
    }

    // free resources
    sem_close(thrd_pl->pool_sem);
    sem_unlink(thrd_pl->pool_name);
    pthread_mutex_destroy(thrd_pl->pool_lock);
    free(thrd_pl->pool_lock);
    free(thrd_pl->tids);
    free(thrd_pl->threads_states);

    return 0;
}