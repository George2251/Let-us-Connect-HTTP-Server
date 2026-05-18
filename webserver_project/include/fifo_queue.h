#ifndef FIFO_QUEUE_H
#define FIFO_QUEUE_H

// Tell the queue that ClientTask exists
struct ClientTask;

#define INVALID_INPUT -1
#define EMPTY_QUEUE -2

// The node now holds our struct ClientTask pointer
typedef struct node {
    struct ClientTask* task;
    struct node* next;
    struct node* prev;
} node_t;

typedef struct {
    int size;
    node_t* first;
    node_t* last;
} fifo_queue_t;

int fifo_queue_init(fifo_queue_t* queue);
int push_last(fifo_queue_t* queue, struct ClientTask* task);
int pop_first(fifo_queue_t* queue, struct ClientTask** task);

#endif // FIFO_QUEUE_H