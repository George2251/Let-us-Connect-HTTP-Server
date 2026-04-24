#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "../../include/fifo_queue.h"
#include "../../include/thread_pool.h"


int thread_pool_init(thread_pool_t* thrd_pl, int max_thread_num, int size_option, const char* pool_name)
{
    if((thrd_pl == NULL) || (max_thread_num < 1)
        || (pool_name == NULL) || ((size_option != THREAD_POOL_SIZE_STATIC)
        && (size_option != THREAD_POOL_SIZE_DYNAMIC)))
                        return INVALID_INPUT;

    thrd_pl = (thread_pool_t*)malloc(sizeof(thread_pool_t));
    thrd_pl->max_size = max_thread_num;

    if((thrd_pl->size_option = size_option) == THREAD_POOL_SIZE_DYNAMIC)
    {
        thrd_pl->curr_size = (max_thread_num < INITIAL_POOL_SIZE)?max_thread_num:INITIAL_POOL_SIZE;
    }
    else
    {
        thrd_pl->curr_size = max_thread_num;
    }

    thrd_pl->available_work = 0;
    thrd_pl->busy_threads_num = 0;
    thrd_pl->pool_name = pool_name;

    if(fifo_queue_init(thrd_pl->queue) != 0)
    {
        free(thrd_pl);
        return ERROR_INITIALIZING_QUEUE;
    } 

    if(pthread_mutex_init(thrd_pl->pool_lock, NULL) != 0)
    {
        fifo_queue_destroy(thrd_pl->queue);
        free(thrd_pl);
        return ERRNO_ERROR;
    }

    if((thrd_pl->pool_sem = sem_open(thrd_pl->pool_name, O_CREAT, 0666, 0)) != 0)
    {
        fifo_queue_destroy(thrd_pl->queue);
        pthread_mutex_destroy(thrd_pl->pool_lock);
        free(thrd_pl);
        return ERRNO_ERROR;
    }


    thrd_pl->threads_states = (thread_state*)malloc(thrd_pl->curr_size*sizeof(thread_state));
    thrd_pl->tids = (pthread_t*)malloc(thrd_pl->curr_size*sizeof(pthread_t));


    pthread_attr_t attr;
    pthread_attr_init(&attr);

    //threads arguments
    thread_args* args = (thread_args*)malloc(sizeof(thread_args));
    args->thrd_pl = thrd_pl;

    sem_t* pool_init_sem = sem_open("INIT_SEM", O_CREAT, 0666, 0);
    args->init_sem = pool_init_sem;     //just for initializing the pool
    for(int i = 0; i < thrd_pl->curr_size; i++)
    {
        thrd_pl->threads_states[i] = FREE;
        args->state = &thrd_pl->threads_states[i];

        pthread_create(&thrd_pl->tids[i], &attr, thread_task, (void*)args);

        //wait for the just created thread to complete its initialization
        sem_wait(pool_init_sem);
    }


    sem_unlink("INIT_SEM");
    free(args);

    return 0;
}


void* thread_task(void* args)
{
    thread_state* state = ((thread_args*)args)->state;
    thread_pool_t* thrd_pl = ((thread_args*)args)->thrd_pl;

    //let the thread pool continue initializing the other threads
    sem_post(((thread_args*)args)->init_sem);

    //do work

}