#pragma once

#include "egos.h"
#include "malloc.h"

/* Note:
 * you should test your code thoroughly.
 * It will be extremely difficult to debug your user-level threading,
 * if there is a bug in your queue implementation.
 */

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
    /* [lab2-ex3]
     * TODO: your code here
     */
    node_t *node = (node_t *) malloc(sizeof(node_t));
    node->item = item;
    node->next = NULL;
    if(q->head == (void *)0) {
        q->head = node;
        q->tail = node;
    }
    else{
        q->tail->next = node;
        q->tail = node;
    }
}

/* remove and return the item at the front of the queue "q".
 * note: you should free memory allocated by enqueue.
 */
void *dequeue(queue_t *q) {
    /* [lab2-ex3]
     * TODO: your code here
     */
    if(q->head == (void *)0) return (void *)0;
    void* item = q->head->item;
    node_t* top = q->head;
    q->head = q->head->next;
    free(top);
    if(q->head == (void *)0){
        q->tail = (void *)0;
    }
    return item;
}

/* return if queue is empty
 */

_Bool empty(queue_t *q) {
    return q->head == NULL;
}

/* print out queue
 *
 */
/*
void print_queue(queue_t *q) {
    node_t* f = q->head;
    while(f){
        printf("%x -> ",(struct thread*)(f->item) -> id);
        f = f->next;
    }
    printf("NULL\n");
}
*/
