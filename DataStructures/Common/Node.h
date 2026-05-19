#ifndef Node_h
#define Node_h

struct Node
{
    void *data;
    struct Node *next;
    struct Node *previous;
};

struct Node node_constructor(void *data, unsigned long size);
void node_destructor(struct Node *node);

#endif /* Node_h */
