#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#define INVALID_INPUT               1
#define ERROR_INITIALIZING_QUEUE    2
#define ERRNO_ERROR                 3

#define THREAD_POOL_SIZE_STATIC     0
#define THREAD_POOL_SIZE_DYNAMIC    1

#define INITIAL_POOL_SIZE           10

typedef enum{
    FREE,
    BUSY,
    TERMINATED,
    maxNumOfStates,
}thread_state;

typedef struct{
    int max_size;
    int curr_size;      //used for dynamic size option, otherwise is the same as max_size
    int available_work;
    int busy_threads_num;
    const char* pool_name;
    int size_option;
    thread_state* threads_states;
    pthread_t* tids;
    fifo_queue_t* queue;
    pthread_mutex_t* pool_lock;
    sem_t* pool_sem;
}thread_pool_t;

typedef struct{
    thread_pool_t* thrd_pl;
    thread_state* state;
    sem_t* init_sem;
}thread_args;


/**
 * @brief initializes the thread pool
 * @param thrd_pl will point to the initialized thread pool
 * @param max_thread_num the maximum number of threads that the thread pool can have
 * @param size_option can be:
 *          THREAD_POOL_SIZE_STATIC: the pool is initialized with this size
 *          THREAD_POOL_SIZE_DYNAMIC: the pool is initialized with minimum of max_thread_num and
 *              INITIAL_POOL_SIZE and the sizes changes based on needs
 * @param pool_name a null terminated character string of the pool name (used to initialize 
 *                  unique semaphores)
 * @return 0 if everything is okay or,
 *         INVALID_INPUT: if any of the inputs have invalid value
 *         ERROR_INITIALIZING_QUEUE: if the queue initialization fails
 *         ERRNO_ERROR: check errno for the error
 */
int thread_pool_init(thread_pool_t* thrd_pl, int max_thread_num, int size_option, const char* pool_name);

int add_work(thread_pool_t* thrd_pl, int client_sockfd);

void* thread_task(void* arg);

int thread_pool_destroy(thread_pool_t* thrd_pl, int option);

#endif