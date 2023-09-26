/*
 * (C) 2023, Cornell University
 * All rights reserved.
 */

/* Author: Robbert van Renesse
 * Description: queue and user-level threading
 * Students implement a queue and a threading package;
 * And then spawn multiple threads as either producer or consumer.
 *
 * Updated by CS6640 23fall staff
 */

#include "app.h"
#include "queue.h"
#include <stdint.h>
// global variable hold the next thread_id;
static uint32_t next_id = 0;
static queue_t* thread_queue;
static queue_t* terminated_threads;
static struct thread* current_thread;

// get the thread id;
uint32_t get_thread_id() {
    return next_id++;
}

/** These two functions are defined in grass/context.S **/
void ctx_start(void** old_sp, void* new_sp);
void ctx_switch(void** old_sp, void* new_sp);

/** Multi-threading functions **/
void thread_create();
void thread_yield();
void thread_exit();

enum thread_status{
    NEW,
    RUNNING,
    WAITING,
    TERMINATED
};

struct thread {
    /* [lab2-ex1]
     * TODO: your code here
     */
    uint32_t id;
    enum thread_status status; // thread's status.
    void (*thread_func)(void*); // pointer to thread's main function.
    void* func_args; // pointer of thread's main function's args
    void* sp; // top of thread's stack
    void* stack_bot; // bot of stack use to free;

};


void thread_init(){
    /* [lab2-ex4]
     * TODO: your code here
     */
    thread_queue = (queue_t*) malloc(sizeof(queue_t));
    //printf("queue malloced\n");
    queue_init(thread_queue);
    terminated_threads = (queue_t*) malloc(sizeof(queue_t));
    queue_init(terminated_threads);
    //printf("queue inited\n");
    current_thread = (struct thread*)malloc(sizeof(struct thread));
    //printf("current_thread malloced\n");
    current_thread->id = get_thread_id();
    current_thread->status = RUNNING;
    current_thread->thread_func = NULL;
    current_thread->func_args = NULL;
    current_thread->sp = NULL;
    current_thread->stack_bot = NULL;
    //printf("thread_init\n");
    //enqueue(thread_queue, tmain);

    //INFO("thread_init is not implemented\n");
    //exit(0);
}


void thread_create(void (*f)(void *), void *arg, unsigned int stack_size) {
    /* [lab2-ex4]
     * TODO: your code here
     * note: stack grows from HIGH to LOW in memory
     */

    //INFO("thread_create is not implemented\n");
    //exit(0);
    void* stack = (void*) malloc(stack_size);
    struct thread* new_thread = (struct thread*)malloc(sizeof(struct thread));
    new_thread->id = get_thread_id();
    new_thread->status = NEW;
    new_thread->thread_func = f;
    new_thread->func_args = arg;
    new_thread->sp = stack + stack_size;
    new_thread->stack_bot = stack;
    
    current_thread->status = WAITING;
    enqueue(thread_queue, current_thread); // push current_thread into runnable queue
    void ** old_sp = &(current_thread->sp); // keep the address of current_thread's sp.
    current_thread = new_thread; // update current_thread with new_thread. (using in ctx_entry())
    // printf("thread%d_create\n", current_thread->id);
    // printf("thread_queue: ");print_queue(thread_queue);
    ctx_start(old_sp, new_thread->sp); // save the current contex, and start new _thread.
}



void ctx_entry(void){
    /* [lab2-ex4]
     * TODO: your code here
     */

    //INFO("ctx_entry is not implemented\n");
    //exit(0);
    //printf("ctx_entry\n");
    current_thread->status = RUNNING;
    current_thread->thread_func(current_thread->func_args);
    thread_exit();
}

void thread_yield(){
    /* [lab2-ex5]
     * TODO: your code here
     */

    // INFO("thread_yield is not implemented\n");
    // exit(0);
    if(empty(thread_queue)) return;
    void ** old_sp = &(current_thread->sp); // keep the address of current_thread's sp.
    enqueue(thread_queue, current_thread); // push current thread into queue.
    current_thread->status = WAITING;
    // printf("thread%d waiting\n", current_thread->id);
    current_thread = dequeue(thread_queue); // update current_thread with new_thread. (using in ctx_entry())
    current_thread->status = RUNNING;
    // printf("thread%d running\n", current_thread->id);
    // printf("thread_queue: ");print_queue(thread_queue);
    ctx_switch(old_sp, current_thread->sp);
}

void thread_exit(){
    /* [lab2-ex5]
     * TODO: your code here
     */
    // INFO("thread_exit is not implemented\n");
    // exit(0);
    
    // printf("thread%d start thread exit\n", current_thread->id);
    while(current_thread->id == 0 && !empty(thread_queue)) { // if there are still runnable threads ru others.
        thread_yield();
    } // let thread 0 as last one to exit.
    while(!empty(terminated_threads)) {
        // printf("current->id: %d\n", current_thread->id);
        // printf("get here\n");
        // printf("terminated->head->item: %x\n", terminated_threads->head);
        struct thread* terminated = dequeue(terminated_threads);
        // printf("terminated id%x\n", terminated->id);
        // print_queue(terminated_threads);
        // printf("terminated thread: %x\n", terminated);
        // printf("terminated threads empty: %d\n", empty(terminated_threads));
        // printf("terminated->id: %d\n", terminated->id);
        if(terminated->stack_bot != NULL){
            free(terminated->stack_bot);
            // printf("stack freed\n");
        
        }
        free(terminated);
        // printf("TCB freed\n");
    }

    void ** old_sp = &(current_thread->sp);
    //printf("old id: %d\n", current_thread->id);
    enqueue(terminated_threads, current_thread);
    // printf("terminated_threads: ");print_queue(terminated_threads);
    //printf("terminated threads empty: %d\n", empty(terminated_threads));
    //printf("pushed to terminated queue\n");
    current_thread->status = TERMINATED;
    //printf("!empty(thread_queue): %d\n", !empty(thread_queue));
    if(!empty(thread_queue)) {
        current_thread = dequeue(thread_queue);
        // printf("current_thread->id: %d\n", current_thread->id);
        ctx_switch(old_sp, current_thread->sp);
        // printf("ctx switched");    
    }
    else{
        // printf("terminated threads empty: %d\n", empty(terminated_threads));
        // print_queue(terminated_threads);
        dequeue(terminated_threads);
        if(current_thread->stack_bot) {
            free(current_thread->stack_bot);
            // printf("stack freed\n");
        }
        free(current_thread);
        // printf("TCB freed\n");
        free(terminated_threads);
        // printf("terminated_threads freed\n");
        free(thread_queue);
        // printf("thread_queue freed\n");
        //exit(0);
    } 
    //printf("terminated threads empty: %d\n", empty(terminated_threads));
    //printf("!empty(terminated_threads): %d\n", !empty(terminated_threads));
    
}

/* print out queue
 *
 */

void print_queue(queue_t *q) {
    node_t* f = q->head;
    while(f){
        printf("%x -> ",((struct thread*)(f->item)) -> id);
        f = f->next;
    }
    printf("NULL\n");
}


/* Main and test cases */
void test_create();
void test_stack();
void test_exit();
void test_single_yield();
void test_pingpong_yield();
void test_producer_consumer();

int main() {
    // INFO("User-level threading is not implemented.");
    // while(1);

    /* test cases for [lab2-ex4]
     * These tests touch thread_init, thread_create, and ctx_entry.
     * You should run one test at a time.
     */
    printf("start test\n");
    //test_create();
    // test_stack();

    /* test case for [lab2-ex5]
     * These tests touch all functions.
     * You should run one test at a time.
     */
    // test_exit();
    // test_single_yield();
    // test_pingpong_yield();
    test_producer_consumer();

    return 0;
}

// ------- test cases ----------

/* Testing thread function */

static void hello(void *arg){
    if (arg == NULL) {
        printf("Hello without arg\n");
    } else {
        printf("Hello with %s\n", arg);
    }
}

static void stack(void *arg){
    static int i = 128;
    while(i>0) {
        --i;
        printf("stak: nested call %d\n", i);
        stack(NULL);
    }

    if (arg != NULL) {
        printf("stack is %s\n", arg);
    }
}

static void many_yield(void *arg) {
    int i = 0;
    for (; i<102400; i++) {
        thread_yield();
    }
    printf("pass: many yield[%s]: i=%d\n", arg, i);
}

/** Producer and consumer functions **/

#define NSLOTS	3

static char *slots[NSLOTS];
static unsigned int in, out;

static void producer(void *arg){
    for (;;) {
        // first make sure there's an empty slot.
        // then add an entry to the slots
        // lastly, let other threads run

        while(slots[in] != (void*)0) {
            thread_yield();
        }

        slots[in++] = arg;
        if (in == NSLOTS) in = 0;
        thread_yield();
    }
}

static void consumer(void *arg){
    for (int i = 0; i < 5; i++) {
        // first make sure there's something in the buffer
        // then grab an entry to the slot
        // lastly, let other threads run

        while(slots[out] == (void*)0) {
            thread_yield();
        }

        void *x = slots[out];
        slots[out] == (void*)0;
        if (++out == NSLOTS) out = 0;
        printf("%s: got '%s'\n", arg, x);
        thread_yield();
    }
}

/* if you pass this, you should see
 *  "Hello with world"
 */
void test_create() {
    printf("start test_create\n");
    thread_init();
    thread_create(hello, "world", 1024);
    thread_exit();
}

/* if you pass this, you should see
 * """
 * stak: nested call 127
 * ...
 * stak: nested call 0
 * stack is fine
 * """
 */
void test_stack() {
    thread_init();
    thread_create(stack, "fine", 16*1024);
    thread_exit();
}

/* if you pass this, you should see:
 * """
 * Hello without arg
 * Hello with world 1
 * Hello with world 2
 * Hello with main
 * """
 */
void test_exit() {
    thread_init();
    thread_create(hello, NULL, 1024);
    thread_create(hello, "world 1", 1024);
    thread_create(hello, "world 2", 1024);
    hello("main");
    thread_exit();
}

/* if you pass this, you should see:
 *  "pass: many yield[single]: i=102400"
 */
void test_single_yield() {
    thread_init();
    thread_create(many_yield, "single", 1024);
    thread_exit();
}

/* if you pass this, you should see:
 * """
 * pass: many yield[ping]: i=102400
 * pass: many yield[pong]: i=102400
 * """
 */
void test_pingpong_yield() {
    thread_init();
    thread_create(many_yield, "ping", 1024);
    thread_create(many_yield, "pong", 1024);
    thread_exit();
}

/* if you pass this, you should see:
 * """
 * consumer 5: got 'producer 1'
 * consumer 5: got 'producer 2'
 * consumer 4: got 'producer 1'
 * consumer 3: got 'producer 1'
 * consumer 2: got 'producer 2'
 * consumer 1: got 'producer 1'
 * consumer 5: got 'producer 1'
 * consumer 4: got 'producer 2'
 * consumer 3: got 'producer 1'
 * consumer 2: got 'producer 1'
 * consumer 1: got 'producer 2'
 * consumer 5: got 'producer 1'
 * consumer 4: got 'producer 1'
 * consumer 3: got 'producer 2'
 * consumer 2: got 'producer 1'
 * consumer 1: got 'producer 1'
 * consumer 5: got 'producer 2'
 * consumer 4: got 'producer 1'
 * consumer 3: got 'producer 1'
 * consumer 2: got 'producer 2'
 * consumer 1: got 'producer 1'
 * consumer 4: got 'producer 1'
 * consumer 3: got 'producer 2'
 * consumer 2: got 'producer 1'
 * consumer 1: got 'producer 1'
 * <then, running forever>
 * """
 */
void test_producer_consumer() {
    thread_init();

    thread_create(consumer, "consumer 1", 1024);
    thread_create(consumer, "consumer 2", 1024);
    thread_create(consumer, "consumer 3", 1024);
    thread_create(consumer, "consumer 4", 1024);
    thread_create(consumer, "consumer 5", 1024);
    thread_create(producer, "producer 1", 1024);
    producer("producer 2");
    thread_exit();
}
