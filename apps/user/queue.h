#pragma once

#include "malloc.h"

typedef struct node_t {
    struct node_t *next;
    void *item;
} node_t;


typedef struct queue_t {
    struct node_t *head;
    struct node_t *tail;
} queue_t;


/* initialize queue head and tail */
void queue_init(queue_t *q) {
    q->head = (void *)0;
    q->tail = (void *)0;
}

/* add the "item" to the tail of the queue "q".
 * note: you should wrap the "item" in a "node_t"*/
void enqueue(queue_t *q, void *item) {

}

/* remove and return the item at the front of the queue "q".
 * note: you should free memory allocated by enqueue.
 */
void *dequeue(queue_t *q) {

}
