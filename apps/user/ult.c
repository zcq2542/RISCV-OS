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

/** These two functions are defined in grass/context.S **/
void ctx_start(void** old_sp, void* new_sp);
void ctx_switch(void** old_sp, void* new_sp);

/** Multi-threading functions **/

struct thread {
    /* Student's code goes here. */
};

void thread_init(){
    /* Student's code goes here */
}

void ctx_entry(void){
    /* Student's code goes here. */
}

void thread_create(void (*f)(void *), void *arg, unsigned int stack_size){
    /* Student's code goes here. */
}

void thread_yield(){
    /* Student's code goes here. */
}

void thread_exit(){
    /* Student's code goes here. */
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

int main() {
    INFO("User-level threading is not implemented.");
    while(1);

    /*
    thread_init();

    thread_create(consumer, "consumer 1", 16 * 1024);
    thread_create(consumer, "consumer 2", 16 * 1024);
    thread_create(consumer, "consumer 3", 16 * 1024);
    thread_create(consumer, "consumer 4", 16 * 1024);
    thread_create(consumer, "consumer 5", 16 * 1024);
    thread_create(producer, "producer 1", 16 * 1024);
    producer("producer 2");
    thread_exit();
    */

    return 0;
}
