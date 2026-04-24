#ifndef FIFO_QUEUE_H
#define FIFO_QUEUE_H


#define INVALID_INPUT       1
#define EMPTY_QUEUE         2
#define NOT_EMPTY_QUEUE     3


typedef struct
{
    int sockfd;
    node_t* next;
    node_t* prev;
}node_t;


typedef struct
{
    int size;
    node_t* first;
    node_t* last;
}fifo_queue_t;


/**
 * @brief Initiates a fifo queue to be used by the threads
 * @param queue will be assigned the address of the initialized fifo_queue_t
 * @return 0: everything is okay,
 *         INVALID_INPUT: invalid value for @p queue
 */
int fifo_queue_init(fifo_queue_t* queue);

/**
 * @brief add a node to the tail of the queue
 * @param queue will have a node added to its tail
 * @param sockfd will be added to the node
 * @return 0: everything is okay, 
 *         INVALID_INPUT: invalid value for @p queue
 * @details must be locked before use so only one thread can update @p queue
 */
int push_last(fifo_queue_t* queue, int sockfd);

/**
 * @brief pops a node from the head of the queue
 * @param queue will have the head poped
 * @param sockfd will be filled with the sockfd of the node
 * @return 0: everything is okay,
 *         INVALID_INPUT: invalid value for @p queue or @p sockfd
 *         EMPTY_QUEUE: @p queue is empty
 * @details must be locked before use so only one thread can update @p queue         
 */
int pop_first(fifo_queue_t* queue, int* sockfd);

/**
 * @brief destroys the queue
 * @param queue will be destroyed/freed
 * @return 0: everything is okay
 *         INVALID_INPUT: invalid value for @p queue
 *         NOT_EMPTY_QUEUE: the queue isn't empty
 */
int fifo_queue_destroy(fifo_queue_t* queue);

#endif