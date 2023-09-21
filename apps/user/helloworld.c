#include "app.h"

void ptr_is_address();
void ins_is_zero_one();

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
    printf("init q\n");
    q->head = (void *)0;
    //q->head = NULL;
    q->tail = (void *)0;
    //q->tail = NULL;
    printf("init fin\n");
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

int main(int args, char **argv) {
    /* [lab1-ex3]
     * TODO: print hello world
     */
    printf("heelo\n");
    printf("%d\n", sizeof(queue_t));
    queue_t* q = (queue_t*)malloc(sizeof(queue_t));
    printf("malloc ok\n");
    queue_init(q);
    printf("defin x1:\n");
    int value = 1;
    int* x1 = &value;
    printf("*x1: %d\n", *x1);
    int value2 = 2;
    int* x2 = &value2; 
    printf("*x2: %d\n", *x2);
    printf("empty: %d\n", empty(q));
    enqueue(q, x1);
    enqueue(q, x2);
    printf("empty: %d\n", empty(q));
    int * p1 = (int*)dequeue(q);
    printf("%d\n", *p1);
    int* p2 = (int*)dequeue(q);
    printf("%d\n", *p2);
    void *p3 = dequeue(q);
    printf("%d\n", (int)p3);
    printf("empty: %d\n", empty(q));

    void *p4 = dequeue(q);
    printf("%d\n", p4);
    enqueue(q, x1);
    enqueue(q, x2);
    p1 = (int*)dequeue(q);
    printf("%d\n", *p1);
    p2 = (int*)dequeue(q);
    printf("%d\n", *p2);
    




}

void ptr_is_address() {
    /* [lab1-ex5]
     * TODO: replace 0x0 with a __valid__ memory address
     */
    int *ptr = (int *)0x0;

    // read value from the ptr
    int val = *ptr;

    // write value to the ptr
    *ptr = val + 1;

    printf("ptr:%p before:%d after:%d\n", ptr, val, *ptr);
}

void ins_is_zero_one() {
    printf("Enter ins_is_zero_one()\n");

    // the instruction address where app exits
    int *exit_code = (int*) 0x820000c;

    /* [lab1-ex6]
     * TODO: replace 0x0 with the __binary__ instruction
     * of "call main".
     * How to know what is this instruction in its binary form?
     * Read "build/debug/helloworld.lst" and search "call main"
     */
    int call_main = 0x0;

    // replace with
    *exit_code = call_main;

    while(1);
}
