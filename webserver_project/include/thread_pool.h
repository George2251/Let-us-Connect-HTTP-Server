#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>    // Required for pthread_t, pthread_mutex_t
#include <semaphore.h>  // Required for sem_t, sem_wait, sem_post
#include "fifo_queue.h"

#define THREAD_POOL_SIZE_STATIC 1
#define THREAD_POOL_SIZE_DYNAMIC 2
#define THREAD_POOL_DESTROY_SOFT 1
#define THREAD_POOL_DESTROY_HARD 2
#define INITIAL_POOL_SIZE 4
#define ERROR_INITIALIZING_QUEUE -2
#define ERRNO_ERROR -3

typedef enum { FREE, BUSY, TERMINATED } thread_state;

typedef struct {
    int max_size;
    int curr_size;
    int size_option;
    int available_work;
    int busy_threads_num;
    const char *pool_name;
    fifo_queue_t queue;
    pthread_mutex_t *pool_lock;
    sem_t *pool_sem;
    thread_state **threads_states;
    pthread_t *tids;
} thread_pool_t;

typedef struct {
    thread_pool_t *thrd_pl;
    thread_state *state;
    sem_t *init_sem;
} thread_args;

int thread_pool_init(thread_pool_t *thrd_pl, int max_thread_num, int size_option, const char *pool_name);
void *_thread_task(void *args);
int add_work(thread_pool_t *thrd_pl, struct ClientTask *task);
int thread_pool_destroy(thread_pool_t *thrd_pl, int option);

#endif // THREAD_POOL_H