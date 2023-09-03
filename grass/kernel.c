/*
 * (C) 2022, Cornell University
 * All rights reserved.
 */

/* Author: Yunhao Zhang
 * Description: interrupt and exception handler
 *
 * Updated by CS6640 23fall staff.
 */


#include "egos.h"
#include "process.h"
#include "syscall.h"
#include <string.h>

#define INTR_ID_SOFT       3
#define INTR_ID_TIMER      7

/* Kernel status */
int proc_curr_idx;
struct process proc_set[MAX_NPROCESS];

void intr_entry(int id) {
    if (id == INTR_ID_TIMER && curr_pid < GPID_SHELL) {
        /* Do not interrupt kernel processes since IO can be stateful */
        earth->timer_reset();
        return;
    }

    if (curr_pid >= GPID_USER_START && earth->tty_intr()) {
        /* User process killed by ctrl+c interrupt */
        INFO("process %d killed by interrupt", curr_pid);
        proc_set[proc_curr_idx].mepc = (void*) (APPS_ENTRY + 0xC);
        return;
    }

    if (id == INTR_ID_SOFT) {
        proc_syscall();
    } else if (id == INTR_ID_TIMER) {
        proc_yield();
    } else
        FATAL("intr_entry: got unknown interrupt %d", id);
}

void excp_entry(int id) {
    FATAL("excp_entry: kernel got exception %d", id);
}

void check_nested_trap() {
    unsigned int sp;
    asm("mv %0, sp" : "=r"(sp));
    if ((GRASS_STACK_TOP - sp) < 2 * PAGE_SIZE) {
        FATAL("trap_handler: meet nested trap in kernel");
    }
}

static unsigned int trap_cause;
void trap_handler(unsigned int mcause) {
    check_nested_trap();

    // save mcause for later use
    trap_cause = mcause;

    /* Switch to the kernel stack */
    ctx_start(&proc_set[proc_curr_idx].sp, (void*)GRASS_STACK_TOP);
    // jump to ctx_entry
}

void ctx_entry() {
    /* Now on the kernel stack */
    int mepc, ignore;
    asm("csrr %0, mepc" : "=r"(mepc));
    proc_set[proc_curr_idx].mepc = (void*) mepc;

    /* handle either interrupt or exception */
    int id = trap_cause & 0x3FF;
    (trap_cause & (1 << 31)) ? intr_entry(id) : excp_entry(id);

    /* Switch back to the user application stack */
    mepc = (int)proc_set[proc_curr_idx].mepc;
    asm("csrw mepc, %0" ::"r"(mepc));
    ctx_switch((void**)&ignore, proc_set[proc_curr_idx].sp);
}

