#pragma once

#include "egos.h"
#include "malloc.h"
//#include "stdio.h"

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
    if(!node) {
        printf("Fail to alloc mem in queue\n");
    }
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

/* [lab3-ex2]
 * the following functions can be useful for your MLFQ.
 */

// check if the "target" is in the queue "q"
int in_queue(queue_t *q, void *target) {
    /* TODO: your code here */
    if(q->head == (void*) 0) return 0;
    node_t* flag = q->head;
    while(flag != NULL){
        if(flag->item == target) return 1;
        flag = flag->next;
    }
    return 0;
}

// try to remove the "target" from the queue "q",
// but the target may or may not in the queue.
int try_rm_item(queue_t *q, void *target) {
    /* TODO: your code here */

    if(q->head == (void*) 0) {
        // printf("q is empty\n");
        return 0;
    }

    if(q->head == q->tail && q->head->item == target){ // q size == 1
        free(q->head);
        q->head = NULL;
        q->tail = NULL;
        return 1;
    }

    node_t* flag = q->head; // q size > 1
    while(flag->next != NULL){
        if(flag->next->item == target){
            if(flag->next == q->tail) {q->tail = flag;};
            node_t* temp = flag->next;
            flag->next = temp->next;
            free(temp);
            return 1;
        }
        flag = flag->next;
    }
    return 0;
}

// remove the "target" from the queue "q",
// the target must be in the queue; otherwise, there is an error
void rm_item(queue_t *q, void *target) {
    /* TODO: your code here */
    if(q->head == q->tail && q->head->item == target){ // q size == 1
        free(q->head);
        q->head = NULL;
        q->tail = NULL;
        return;
    }
    if(q->head->item == target){
        node_t* temp = q->head;
        q->head = temp->next;
        free(temp);
        return;
    }

    node_t* flag = q->head; // q size > 1
    while(flag->next != NULL){
        if(flag->next->item == target){
            if(flag->next == q->tail) {q->tail = flag;};
            node_t* temp = flag->next;
            flag->next = temp->next;
            free(temp);
            return;
        }
        flag = flag->next;
    }
}

// print the queue with whatever information you think is useful
void dump_queue(queue_t *q) {
    /* TODO: your code here */
    // if(q->head == (void*) 0) printf("q is empty\n");
    node_t* flag = q->head;
    while(flag != NULL){
        printf("%x -> ", flag->item);
        flag = flag->next;
    }
    printf("NULL\n");
}
