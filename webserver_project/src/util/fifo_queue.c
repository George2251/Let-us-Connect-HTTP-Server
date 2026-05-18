#include "../../include/fifo_queue.h"
#include <stdlib.h>     //for malloc and free


int fifo_queue_init(fifo_queue_t* queue)
{
    queue->size=0;
    queue->first=NULL;
    queue->last=NULL;

    return 0;
}


int push_last(fifo_queue_t* queue, struct ClientTask* task)
{
    if(queue == NULL) return INVALID_INPUT;

    node_t* newNode = (node_t*)malloc(sizeof(node_t));
    newNode->task=task;
    newNode->prev = NULL;

    if(queue->size == 0)
    {
        newNode->next = NULL;
        queue->first = newNode;
        queue->last = newNode;
    }
    else
    {
        queue->last->prev = newNode;
        newNode->next = queue->last;
        queue->last = newNode;
    }

    queue->size++;

    return 0;
}


int pop_first(fifo_queue_t* queue, struct ClientTask** task)
{
    if((queue == NULL) || (task == NULL)) return INVALID_INPUT; 

    if((queue->size == 0) || (queue->first == NULL)) return EMPTY_QUEUE;

    node_t* oldNode = queue->first;
    *task = oldNode->task;
    queue->first = oldNode->prev;

    free(oldNode);

    queue->size--;

    if(queue->size == 0)
    {
        queue->last = NULL;
    }

    if(queue->size != 0)
    {
        queue->first->next = NULL;
    }

    return 0;
}
