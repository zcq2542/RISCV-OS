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
    /*
     * [lab4-ex2]
     * TODO: handling exceptions
     * - If "id" is for syscalls, handle the system call and return:
     *   -- you need to capture **all** ecall exceptions
     *   -- you need to think of which pc the CPU will run after "mret";
     *      in other word, you need to properly update PCBs so that
     *      eventually "mepc" will be set to the right instruction.
     * - If "id" is not for syscalls, then check
     *   -- if curr_pid is a user application, kill the process
     *   -- if curr_pid is a system proc, panic the kernel using FATAL
     */
    proc_set[proc_curr_idx].mepc += 4; // mepc set to address after ecall.
    if(id == 8){ // environment call from U-mode
        proc_syscall();
    }
    else if(id == 9){ // environment call from S-mode
        proc_syscall();
    }
    else if(id == 11){ // environment call from M-mode
        proc_syscall();
    }
    else{
        printf("else exception\n");
        if(curr_pid < GPID_USER_START){
            FATAL("fatal exception (pid=%d) %d", curr_pid, id);
        }
        else{
            INFO("process %d killed by exception %d", curr_pid, id);
            proc_set[proc_curr_idx].mepc = (void*) (APPS_ENTRY + 0xC);
            return; //asm("mret");
        }
    }

    //printf("excep_entry: \n");
    //FATAL("fatal exception (pid=%d) %d", curr_pid, id);
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
    //printf("trap handler: curr_pid: %d\n Interrupt: %d Exception code: %d\n" , curr_pid, mcause & (1 << 31), mcause & 0x3FF);
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

    /* [lab4-ex3]
     * TODO: the kernel will switch privilege level here:
     * - if the curr_pid is a system process, set the privilege level to S-Mode
     * - if the curr_pid is a user application, set the privilege level to U-Mode
     */
    /* TODO: your code here */
    unsigned long mstatus;
    asm("csrr %0, mstatus" : "=r" (mstatus));
    if(curr_pid < GPID_USER_START){
        mstatus &= ~(0x2 >> 11); // &= 0111111111111
        mstatus |= (0x1 << 11); // |= 0100000000000
    }
    else {
        mstatus &= ~(0x3 << 11); // &= 0011111111111
    }
    asm("csrw mstatus, %0" :: "r" (mstatus));
    

    /* Switch back to the user application stack */
    mepc = (int)proc_set[proc_curr_idx].mepc;
    asm("csrw mepc, %0" ::"r"(mepc));
    ctx_switch((void**)&ignore, proc_set[proc_curr_idx].sp);
}

