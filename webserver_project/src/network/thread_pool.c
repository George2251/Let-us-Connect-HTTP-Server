#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "../../include/fifo_queue.h"
#include "../../include/thread_pool.h"

int thread_pool_init(thread_pool_t *thrd_pl, int max_thread_num, int size_option, const char *pool_name)
{
    if ((thrd_pl == NULL) || (max_thread_num < 1) || (pool_name == NULL) || ((size_option != THREAD_POOL_SIZE_STATIC) && (size_option != THREAD_POOL_SIZE_DYNAMIC)))
        return INVALID_INPUT;

    thrd_pl = (thread_pool_t *)malloc(sizeof(thread_pool_t));
    thrd_pl->max_size = max_thread_num;

    if ((thrd_pl->size_option = size_option) == THREAD_POOL_SIZE_DYNAMIC)
    {
        thrd_pl->curr_size = (max_thread_num < INITIAL_POOL_SIZE) ? max_thread_num : INITIAL_POOL_SIZE;
    }
    else
    {
        thrd_pl->curr_size = max_thread_num;
    }

    thrd_pl->available_work = 0;
    thrd_pl->busy_threads_num = 0;
    thrd_pl->pool_name = pool_name;

    if (fifo_queue_init(thrd_pl->queue) != 0)
    {
        free(thrd_pl);
        return ERROR_INITIALIZING_QUEUE;
    }

    if (pthread_mutex_init(thrd_pl->pool_lock, NULL) != 0)
    {
        fifo_queue_destroy(thrd_pl->queue);
        free(thrd_pl);
        return ERRNO_ERROR;
    }

    if ((thrd_pl->pool_sem = sem_open(thrd_pl->pool_name, O_CREAT, 0666, 0)) != 0)
    {
        fifo_queue_destroy(thrd_pl->queue);
        pthread_mutex_destroy(thrd_pl->pool_lock);
        free(thrd_pl);
        return ERRNO_ERROR;
    }

    thrd_pl->threads_states = (thread_state **)malloc(thrd_pl->curr_size * sizeof(thread_state *));
    thrd_pl->tids = (pthread_t *)malloc(thrd_pl->curr_size * sizeof(pthread_t));

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    // threads arguments
    thread_args *args = (thread_args *)malloc(sizeof(thread_args));
    args->thrd_pl = thrd_pl;

    sem_t *pool_init_sem = sem_open("INIT_SEM", O_CREAT, 0666, 0);
    args->init_sem = pool_init_sem; // just for initializing the pool
    for (int i = 0; i < thrd_pl->curr_size; i++)
    {
        thrd_pl->threads_states[i] = (thread_state *)malloc(sizeof(thread_state));
        *(thrd_pl->threads_states[i]) = FREE;
        args->state = thrd_pl->threads_states[i];

        pthread_create(&thrd_pl->tids[i], &attr, thread_task, (void *)args);

        // wait for the just created thread to complete its initialization
        sem_wait(pool_init_sem);
    }

    sem_unlink("INIT_SEM");
    free(args);

    return 0;
}

void *thread_task(void *args)
{
    thread_state *state = ((thread_args *)args)->state;
    thread_pool_t *thrd_pl = ((thread_args *)args)->thrd_pl;

    // let the thread pool continue initializing the other threads
    sem_post(((thread_args *)args)->init_sem);

    while (1)
    {
        sem_wait(thrd_pl->pool_sem);

        if (*state == TERMINATED)
        {
            break;
        }

        pthread_mutex_lock(thrd_pl->pool_lock);

        if (thrd_pl->available_work > 0)
        { // if there is work to do
            int sockfd;
            pop_first(thrd_pl->queue, &sockfd);
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

        //*******************************************************************/
        // end of work

        if (*state == TERMINATED)
        {
            break;
        }

        pthread_mutex_lock(thrd_pl->pool_lock);

        thrd_pl->busy_threads_num--;
        *state = FREE;

        pthread_mutex_unlock(thrd_pl->pool_lock);
    }

    pthread_exit(0);
}

int add_work(thread_pool_t *thrd_pl, int client_sockfd)
{
    if (thrd_pl == NULL)
        return INVALID_INPUT;

    if (thrd_pl->size_option == THREAD_POOL_SIZE_STATIC)
    { // size is static, just add the work
        pthread_mutex_lock(thrd_pl->pool_lock);

        push_last(thrd_pl->queue, client_sockfd);
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
    if ((thrd_pl == NULL) || ((option != THREAD_POOL_DESTROY_SOFT) && (option == THREAD_POOL_DESTROY_HARD)))
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
    fifo_queue_destroy(thrd_pl->queue);
    free(thrd_pl->tids);
    free(thrd_pl->threads_states);
    free(thrd_pl);

    return 0;
}