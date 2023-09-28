/*
 * Description: a simple timer example
 */

#include "egos.h"
struct earth *earth = (void*)GRASS_STACK_TOP;

#define CLINT0_MTIME     0x200bff8
#define CLINT0_MTIMECMP  0x2004000

long long mtime_get() {
    int low  = *(int*)(CLINT0_MTIME);
    int high = *(int*)(CLINT0_MTIME + 4);
    return (((long long)high) << 32) | low;
}

void mtimecmp_set(long long time) {
    *(int*)(CLINT0_MTIMECMP + 0) = (int)time;
    *(int*)(CLINT0_MTIMECMP + 4) = (int)(time >> 32);
}

void handler() {
    static long long last_time = 0;
    CRITICAL("Got a timer interrupt!");

    if (mtime_get() < last_time) {
        printf("last time: 0x%llx, this time: 0x%llx\n",
                last_time, mtime_get());
        FATAL("timer overflow");
    }
    last_time = mtime_get();

    // (4) reset timer
    mtimecmp_set(mtime_get() + QUANTUM);
}


int main() {
    CRITICAL("This is a simple timer example");

    // (1) register handler() as interrupt handler
    asm("csrw mtvec, %0" ::"r"(handler));

    // (2) Set a timer
    mtimecmp_set(mtime_get() + QUANTUM);

    // (3) enable timer interrupt
    int mstatus, mie;
    asm("csrr %0, mstatus" : "=r"(mstatus));
    asm("csrw mstatus, %0" ::"r"(mstatus | 0x8));
    asm("csrr %0, mie" : "=r"(mie));
    asm("csrw mie, %0" ::"r"(mie | 0x80));


    while(1);
}
