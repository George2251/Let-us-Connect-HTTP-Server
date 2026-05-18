#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <sys/socket.h>  // For recv()
#include <string.h>      // For strlen()
#include <unistd.h>        // For close()

#include "../../include/thread_pool.h"
#include "../../include/network.h"
#include "../../include/http_parser.h"

int thread_pool_init(thread_pool_t *thrd_pl, int max_thread_num, int size_option, const char *pool_name)
{
    //check for invalid input
    if ((max_thread_num < 1) || (pool_name == NULL) || ((size_option != THREAD_POOL_SIZE_STATIC) && (size_option != THREAD_POOL_SIZE_DYNAMIC)))
        return INVALID_INPUT;

    thrd_pl->max_size = max_thread_num;

    //check whether to choose the curr_size based on the size_option
    //static size means that the size is constant
    //dynamic size means that the size changes based on demand (NOT implemented yet)
    if ((thrd_pl->size_option = size_option) == THREAD_POOL_SIZE_DYNAMIC)
    {
        thrd_pl->curr_size = (max_thread_num < INITIAL_POOL_SIZE) ? max_thread_num : INITIAL_POOL_SIZE;
    }
    else
    {
        thrd_pl->curr_size = max_thread_num;
    }

    //set the task-related members to reset values
    thrd_pl->available_work = 0;
    thrd_pl->busy_threads_num = 0;

    //the pool name is used for the semaphore
    thrd_pl->pool_name = pool_name; 

    //initialize the queue that would carry the tasks
    if (fifo_queue_init(&thrd_pl->queue) != 0)
    {
        return ERROR_INITIALIZING_QUEUE;
    }

    //initialize the lock that would be used to protect the data structure
    //from race conditions while editing it
    thrd_pl->pool_lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (pthread_mutex_init(thrd_pl->pool_lock, NULL) != 0)
    {
        return ERRNO_ERROR;
    }

    //initialize the semaphore which will wake threads when ever ther is work
    if ((thrd_pl->pool_sem = sem_open(thrd_pl->pool_name, O_CREAT, 0666, 0)) == SEM_FAILED)
    {
        pthread_mutex_destroy(thrd_pl->pool_lock);
        return ERRNO_ERROR;
    }


    //initialize the thread info related arrays
    thrd_pl->threads_states = (thread_state **)malloc(thrd_pl->curr_size * sizeof(thread_state *));
    thrd_pl->tids = (pthread_t *)malloc(thrd_pl->curr_size * sizeof(pthread_t));

    //initialize the thread attributes
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    // threads arguments, which will be packaged and sent to the thread
    thread_args *args = (thread_args *)malloc(sizeof(thread_args));
    args->thrd_pl = thrd_pl;

    //this semaphore will be used only to initialize each thread
    sem_t *pool_init_sem = sem_open("INIT_SEM", O_CREAT, 0666, 0);
    args->init_sem = pool_init_sem; // just for initializing the pool
    for (int i = 0; i < thrd_pl->curr_size; i++)
    {

        //give each thread a pointer to its state information so 
        //the both the main thread and child thread can read and write the state
        thrd_pl->threads_states[i] = (thread_state *)malloc(sizeof(thread_state));
        *(thrd_pl->threads_states[i]) = FREE;
        args->state = thrd_pl->threads_states[i];

        pthread_create(&thrd_pl->tids[i], &attr, _thread_task, (void *)args);

        // wait for the just created thread to complete its initialization
        sem_wait(pool_init_sem);
    }

    //destroy those since they are only for initialization
    sem_unlink("INIT_SEM");
    free(args);

    return 0;
}

void *_thread_task(void *args)
{
    //give the thread pointers to its state and thread pool
    thread_state *state = ((thread_args *)args)->state;
    thread_pool_t *thrd_pl = ((thread_args *)args)->thrd_pl;

    // let the thread pool continue initializing the other threads
    sem_post(((thread_args *)args)->init_sem);

    while (1)
    {
        //wait here untill a work is available
        sem_wait(thrd_pl->pool_sem);

        //check if the main thread demands that this thread 
        //to be closed
        if (*state == TERMINATED)
        {
            break;
        }

        //lock the thread pool so no race conditions happen
        pthread_mutex_lock(thrd_pl->pool_lock);

        struct ClientTask *task = NULL; // this struct will carry the client socket and the routes dictionary, it is allocated in network.c when a new client connects and is added to the queue as work for the thread pool, here we pop it from the queue and use its data to handle the request


        //int sockfd;

        if (thrd_pl->available_work > 0)
        { // if there is work to do

            //get the work and edit the thread pool work-related data members
            pop_first(&thrd_pl->queue, &task); // pop the work, this should give us the client socket and the routes dictionary that we need to handle the request
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

        if(task!=NULL){
            char buffer[9182]={0};// buffer to hold the incoming HTTP request
            // Read from the socket. Using MSG_NOSIGNAL prevents crashing if the client disconnects early.
            ssize_t bytes_read = recv(task->client_fd, buffer, sizeof(buffer) - 1, MSG_NOSIGNAL);
            if (bytes_read > 0) {
                // 1. Construct the HTTP request dictionary from the raw text
                struct HTTPRequest req = http_request_constructor(buffer);

                // 2. Extract the requested URI
                char* uri = (char*)req.request_line.search(&req.request_line, "uri", sizeof("uri"));
                
                // FIX: Look up a pointer to the handler, not the handler itself
                route_handler_t *handler_ptr = NULL; 
                 
                // 3. Search the routes dictionary
                if (uri != NULL && task->routes != NULL) {
                    handler_ptr = (route_handler_t*)task->routes->search(task->routes, uri, strlen(uri) + 1);
                }
                
                // 4. If a route matches, dereference and execute. Otherwise, send a 404.
                if (handler_ptr != NULL) {
                    (*handler_ptr)(task->client_fd, &req);
                } 
                else{
                    send_error(task->client_fd, 404);
                }
                
                // 5. Deallocate the dictionaries inside the request struct
                http_request_destructor(&req);
            }
             // 6. Close the socket when finished
            close(task->client_fd);
            
            // 7. Free the memory for the task that was allocated back in network.c
            free(task);
        }   


        //*******************************************************************/
        // end of work

        //check if the main thread demands that this thread 
        //to be closed, the reason that this is repeated that the state
        //would be written by the thread later
        if (*state == TERMINATED)
        {
            break;
        }

        //lock the thread pool again to reflect that this thread
        //is no longer busy doing work
        pthread_mutex_lock(thrd_pl->pool_lock);

        thrd_pl->busy_threads_num--;
        *state = FREE;

        pthread_mutex_unlock(thrd_pl->pool_lock);
    }

    pthread_exit(0);
}

int add_work(thread_pool_t *thrd_pl, struct ClientTask *task)
{
    //check for the input
    if (thrd_pl == NULL)
        return INVALID_INPUT;

    //choose how to add work based on the size_option
    if (thrd_pl->size_option == THREAD_POOL_SIZE_STATIC)
    { // size is static, just add the work
        pthread_mutex_lock(thrd_pl->pool_lock);

        push_last(&thrd_pl->queue, task); // add the work to the queue, this should be the client socket and the routes dictionary that we need to handle the request
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
    //check for inputs
    if ((thrd_pl == NULL) || ((option != THREAD_POOL_DESTROY_SOFT) && (option != THREAD_POOL_DESTROY_HARD)))
        return INVALID_INPUT;

    if (option == THREAD_POOL_DESTROY_SOFT)
    { // wait till every thread has finished its work

        // tell every thread that it is terminated and should exit
        for (int i = 0; i < thrd_pl->curr_size; i++)
            *(thrd_pl->threads_states[i]) = TERMINATED;

        // signal for every thread, more signals isn't bad
        for (int i = 0; i < thrd_pl->curr_size; i++)
            sem_post(thrd_pl->pool_sem);
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
    pthread_mutex_destroy(thrd_pl->pool_lock);
    free(thrd_pl->pool_lock);
    free(thrd_pl->tids);
    free(thrd_pl->threads_states);

    return 0;
}