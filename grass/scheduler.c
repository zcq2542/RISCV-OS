/*
 * (C) 2022, Cornell University
 * All rights reserved.
 */

/* Author: Yunhao Zhang
 * Description: scheduler
 *
 * updated by CS6640 23fall staff
 */

#include "egos.h"
#include "process.h"
#include <string.h>
#include "queue2.h"

/* processes that have pid < USER_PID_START is system processes */
#define USER_PID_START 5
//#define USER_IDX_START pid2idx(USER_PID_START)
#define USER_IDX_START 4


/* translating between pid and idx (in proc_set) */
//#define pid2idx(pid)  ((pid>=1 && pid<=MAX_NPROCESS) ? (pid-1) \
                            : FATAL("pid2idx: invalid pid"))
//#define idx2pid(idx)  ((idx>=0 && idx<MAX_NPROCESS) ? (idx+1) \
                            : FATAL("idx2pid: invalid idx"))

/* [lab3-ex1]
 * TODO: plan how to use "schd_attr"
 *  - You need to design you own
 *  - Below are two examples.
 *  - With these macros, one can get the attribute by, for example,
 *       proc_set[pid2idx(pid)].arrive_time = earth->gettime();
 */
#define arrive_time           schd_attr.longlongs[0]
#define last_schded_time      schd_attr.longlongs[1]
#define arrive_ofcount        schd_attr.longlongs[2] // overflow count when arrive
#define last_schded_ofcount   schd_attr.longlongs[3] // overflow count when last schded
#define wake_up_time          schd_attr.longlongs[4] // if sleeping, wake up time 
#define wake_up_ovc           schd_attr.longlongs[5] // if sleeping, wake up time 

#define schded_count          schd_attr.ints[15]

#define turnaround_time       schd_attr.floats[12] // arrive time -> stop time
#define response_time         schd_attr.floats[13] // arrive time -> 1st scheduled time
#define cpu_time              schd_attr.floats[14] // 

#define MAX_VALUE  0xffffffffffffffff
#define ERA (float)MAX_VALUE / QUANTUM // 1 ERA is the time of 1 period of timer in  unit of QUANTUM
// #define ERA 2513169547264.0

static int mlfq();
static queue_t hq;
static queue_t mq;
static queue_t lq;

int pid2idx(int pid) {
    return ((pid>=1 && pid<=MAX_NPROCESS) ? (pid-1) : FATAL("pid2idx: invalid pid"));
}

// int idx2pid(int idx)  ((idx>=0 && idx<MAX_NPROCESS) ? (idx+1) \
                            : FATAL("idx2pid: invalid idx"))


float time_gap(unsigned long long current_time, unsigned long long last_time, unsigned long long cur_ovflow_count, unsigned long long last_ovflow_count){
    unsigned long long overflow_times = cur_ovflow_count - last_ovflow_count;
    if(cur_ovflow_count < last_ovflow_count){
        FATAL("overflow_count overflowi\n");
    }
    if(current_time <= last_time){ // must overflow
        // printf("%f\n", (float)(MAX_VALUE - last_time + current_time )/QUANTUM);
        // return ((float)((MAX_VALUE - last_time + current_time )/QUANTUM) + ERA * (overflow_times-1));
        return ((float)(MAX_VALUE - last_time + current_time) / QUANTUM + ERA * (overflow_times-1));
        // return ((float)(MAX_VALUE - last_time + current_time)/QUANTUM);
    }
    else{
        // printf("max / quantun: %f\n", (float)MAX_VALUE / QUANTUM * overflow_times);
        // printf("%f\n", (float)(current_time - last_time)/QUANTUM);
        return ((float)(current_time - last_time) / QUANTUM + ERA * (overflow_times));
        // return (current_time - last_time) / (float)QUANTUM;
    }

}

/*
static float time_gap(unsigned long long current_time, unsigned long long last_time) {
    if(current_time <= last_time){ // must overflow
        return (float)(MAX_VALUE - last_time + current_time) / QUANTUM;
    }
    else{
        return (float)(current_time - last_time) / QUANTUM;
    }

}
*/
/* schedule next process */
void proc_yield() {
    /* [lab3-ex1]
     * TODO: update "schd_attr" in "proc_set" for the current process
     * hints:
     *   - the current process will likely to be scheduled "out"
     *   - you can use earth->gettime() to get the current time
     *     -- the returned time is in "long long" (i.e., "m_uint64")
     *   - you may want to calculate how long it runs here
     *     -- a challenge is that the returned time may overflow
     * */
    // printf(" unsigned long long overflow_count = %llx\n", earth->overflow_count);
    // unsigned long long current_time = earth->gettime();
    // unsigned long long last_time = proc_set[proc_curr_idx].last_schded_time;
    // unsigned long long cur_ovflow_count = earth->overflow_count;
    // unsigned long long last_ovflow_count = proc_set[proc_curr_idx].last_schded_ofcount;

    // proc_set[proc_curr_idx].cpu_time += time_gap(current_time, last_time, cur_ovflow_count, last_ovflow_count); 
    proc_set[proc_curr_idx].cpu_time += time_gap(earth->gettime(), proc_set[proc_curr_idx].last_schded_time, earth->overflow_count, proc_set[proc_curr_idx].last_schded_ofcount); 
    //proc_set[pid2idx(curr_pid)].cpu_time += time_gap(current_time, last_time);
    // printf("%f\n", (float)(current_time - last_time) / QUANTUM);
    // printf("pid: %d , %f\n", curr_pid, proc_set[pid2idx(curr_pid)].cpu_time);
    // proc_set_runnable(curr_pid); //cause error.
    /* Find the next runnable process */
    int next_idx = -1;

#ifndef MLFQ // if not defined MLFQ in Makefile
    /* default scheduler policy: find the next runnable process */
    for (int i = 1; i <= MAX_NPROCESS; i++) {
        int s = proc_set[(proc_curr_idx + i) % MAX_NPROCESS].status;
        if (s == PROC_READY || s == PROC_RUNNING || s == PROC_RUNNABLE) {
            next_idx = (proc_curr_idx + i) % MAX_NPROCESS;
            break;
        }
    }
#else
    int next_pid = mlfq();
    // printf("next_pid: %d\n", next_pid);
    next_idx = (next_pid > 0) ? pid2idx(next_pid) : -1;
#endif

    if (next_idx == -1) FATAL("proc_yield: no runnable process");
    if (curr_status == PROC_RUNNING) proc_set_runnable(curr_pid);

    /* Switch to the next runnable process and reset timer */
    proc_curr_idx = next_idx;
    //ASSERT(curr_pid > 0 && curr_pid < pid2idx(MAX_NPROCESS), \
            "proc_yield: invalid pid to switch");
    // if(!(curr_pid > 0 && curr_pid < pid2idx(MAX_NPROCESS))) printf("proc_yield: invalid pid to switch");
    earth->mmu_switch(curr_pid);
    earth->timer_reset();

    /* [lab3-ex1]
     * TODO: update "schd_attr" for the next process (new process)
     * */
    /*
    current_time = earth->gettime();
    cur_ovflow_count = earth->overflow_count;
    last_time = proc_set[proc_curr_idx].arrive_time;
    last_ovflow_count = proc_set[proc_curr_idx].arrive_ofcount;

    */
    proc_set[proc_curr_idx].last_schded_time = earth->gettime();
    proc_set[proc_curr_idx].last_schded_ofcount = earth->overflow_count;
    if(proc_set[proc_curr_idx].schded_count == 0){
        proc_set[proc_curr_idx].response_time = time_gap(proc_set[proc_curr_idx].last_schded_time, proc_set[proc_curr_idx].arrive_time, proc_set[proc_curr_idx].last_schded_ofcount, proc_set[proc_curr_idx].arrive_ofcount);
        // proc_set[pid2idx(curr_pid)].response_time = time_gap(current_time, last_time); 
    }
    ++proc_set[proc_curr_idx].schded_count;





    /* Call the entry point for newly created process */
    if (curr_status == PROC_READY) {
        proc_set_running(curr_pid);

        /* [lab4-ex3]
         * TODO: the kernel will switch privilege level here:
         * - if the curr_pid is a system process, set the privilege level to S-Mode
         * - if the curr_pid is a user application, set the privilege level to U-Mode
         */

        /* TODO: your code here */

        /* Prepare argc and argv */
        asm("mv a0, %0" ::"r"(APPS_ARG));
        asm("mv a1, %0" ::"r"(APPS_ARG + 4));
        /* Enter application code entry using mret */
        asm("csrw mepc, %0" ::"r"(APPS_ENTRY));
        asm("mret");
    }

    proc_set_running(curr_pid);
}

/* [lab3-ex2]
 * initialize MLFQ data structures */
void mlfq_init() {
    /* TODO: your code here*/
    
    // FATAL("mlfq_init not implemented");
    //hq = (queue_t*) malloc (sizeof(queue_t));
    queue_init(&hq);

    //mq = (queue_t*) malloc (sizeof(queue_t));
    queue_init(&mq);
    
    //lq = (queue_t*) malloc (sizeof(queue_t));
    queue_init(&lq);
     
    /*for(int pid = 1; pid < USER_PID_START; ++pid){
        enqueue(&hq, (void*)pid);
    }*/
    
    // printf("mlfq\n");
}

int aval_pid(queue_t* q){
    if(q->head == NULL) return -1;
    int next_pid = -1;
    node_t* flag = q->head;
    while(flag != NULL){
        int pididx = pid2idx((int)(flag->item));
        int s = proc_set[pididx].status;
        if(s == PROC_SLEEPING && proc_set[pididx].wake_up_ovc >= earth->overflow_count && proc_set[pididx].wake_up_time <= earth->gettime()){
            proc_set[pididx].status = PROC_RUNNABLE;
        } 
        if(s == PROC_READY || s == PROC_RUNNING || s == PROC_RUNNABLE){
            next_pid = (int)(flag->item);
            break;
        }
        //else{
            //printf("pid: %d, status: %d\n", (int)flag->item, s);
        //}
        flag = flag->next;
    }
    try_rm_item(q, (void*)next_pid);
    return next_pid;
}

/* [lab3-ex2]
 * implement MLFQ scheduler
 *   - return the **pid** (NOT index) of the next runnable process.
 *   - if there is no other runnable processes and the current process is PROC_RUNNING,
 *     you should let the current process keep running.
 *   - if no process is runnable, return 0.
 *   - you should always put system processes (pid<USER_PID_START) in the highest priority queue
 *   - you should not enqueue PROC_UNUSED
 *
 * hints:
 *   - you need to check the status of the process before running it;
 *     a process might not be runnable when dequeue (for example, in a status of waiting)
 *   - some invariants you want to check (for robust MLFQ implementation):
 *     -- the current running process should not be in the queue
 *     -- all processes except the current running one are in some queue
 *     -- there are no duplicated pids between queues
 *   - use pid2idx() and idx2pid() to translate pid and index in "proc_set"
 * */

static int mlfq() {
    /* TODO: your code here */
    // printf("mlfq()\n");
    /*
    printf("hq: ");
    dump_queue(&hq);
    printf("mq: ");
    dump_queue(&mq);
    printf("lq: ");
    dump_queue(&lq);
    */
    //try_rm_item(&hq, (void*)curr_pid);
    //try_rm_item(&mq, (void*)curr_pid);
    //try_rm_item(&lq, (void*)curr_pid);

    int next_pid = aval_pid(&hq);
    //printf("before enqueue, next_pid: %d\n", next_pid);
    if(next_pid <= 0)  
        next_pid = aval_pid(&mq);
    //printf("after find in mq, next_pid: %d\n", next_pid);
    if(next_pid <= 0)  
        next_pid = aval_pid(&lq);
    //printf("after find in lq, next_pid: %d\n", next_pid);
    
    if(proc_set[proc_curr_idx].status != PROC_UNUSED){
        if(curr_pid < USER_PID_START) {
            //printf("curr_pid: %d push to hq\n", curr_pid);
            enqueue(&hq, (void*)curr_pid);
            //printf("hq: ");
            //dump_queue(&hq);
        }
        else if(proc_set[proc_curr_idx].cpu_time < 1){
            //printf("curr_pid: %d push to hq\n", curr_pid);
            enqueue(&hq, (void*)curr_pid);
            //printf("hq: ");
            //dump_queue(&hq);
        }
        else if(proc_set[proc_curr_idx].cpu_time >= 1 && proc_set[proc_curr_idx].cpu_time < 2){
            //printf("curr_pid: %d push to mq\n", curr_pid);
            enqueue(&mq, (void*)curr_pid);
        }
        else if(proc_set[proc_curr_idx].cpu_time >= 2){
            //printf("curr_pid: %d push to lq\n", curr_pid);
            enqueue(&lq, (void*)curr_pid);
        }
    }
    
    while(next_pid <= 0){
    
        // if(next_pid <= 0) 
            next_pid = aval_pid(&hq);
        if(next_pid <= 0)  
            next_pid = aval_pid(&mq);
        if(next_pid <= 0)  
            next_pid = aval_pid(&lq);
    }

    // printf("next_pid: %d\n", next_pid);

    return next_pid>0 ? next_pid : 0;
  
    //return 0;
}


/* process scheduling metrics */

/* [lab-ex1]
 * TODO: return turn-around time for pid
 * - the returned value is a float number (how many QUANTUM)
 */
static float tar_time(int pid) {
    return proc_set[pid2idx(pid)].turnaround_time;
}

/* [lab-ex1]
 * TODO: return response time for pid
 * - the returned value is a float number (how many QUANTUM)
 */
static float resp_time(int pid) {
    //printf("%llx\n", proc_set[pid2idx(curr_pid)].response_time);
    //printf("%f\n", (float)proc_set[pid2idx(curr_pid)].response_time);
    //printf("QUANTUM: %x\n", QUANTUM);
    //printf("QUANTUM: %f\n", (float)QUANTUM);
    //printf("%f\n", (float)proc_set[pid2idx(curr_pid)].response_time / QUANTUM);
    return proc_set[pid2idx(pid)].response_time;
}

/* [lab-ex1]
 * TODO: return how many times the process pid has been scheduled
 * - the returned value is an integer
 */
static int yield_num(int pid) {
  return proc_set[pid2idx(pid)].schded_count;
}

/* [lab-ex1]
 * TODO: return the actual CPU time the pid used
 * - the returned value is a float number (how many QUANTUM)
 */
static float cpu_runtime(int pid) {
  return proc_set[pid2idx(pid)].cpu_time;
}


/* process life-cycle functions */

/* proc_on_arrive will be called when the process is created */
void proc_on_arrive(int pid) {
    /* [lab-ex1]
     * TODO: collect pid's scheduling information
     * hint: remember to init/clear the pid's "schd_attr"
     */
    int pididx = pid2idx(pid);
    memset(&(proc_set[pididx].schd_attr), 0, sizeof(proc_set[pididx].schd_attr));  // Set all bytes of schd_attr to zer
    // printf("init arrive_time: %f\n", proc_set[pid2idx(pid)].arrive_time);
    // printf("init schded_time: %d\n", proc_set[pid2idx(pid)].schded_count);
    // printf("init cpu_time: %f\n", proc_set[pid2idx(pid)].cpu_time);
    proc_set[pididx].arrive_time = earth->gettime();
    proc_set[pididx].arrive_ofcount = earth->overflow_count;


#ifdef MLFQ
  static int first_time = 1;
  if (first_time) {
      mlfq_init();
      first_time = 0;
      /* return without adding proc to queue or the first time
       * because this process is sys_proc, which will directly
       * run without go via proc_yield().*/
      return;
  }

  /* [lab3-ex2]
   * TODO: add the process to MLFQ
   * Note that pid is not the curr_pid
   */
   // printf("pid: %d arrive\n", pid);
   enqueue(&hq, (void*)pid); 




#endif
}

/* proc_on_stop() will be called when the process exits */
void proc_on_stop(int pid) {
    //ASSERTX(proc_set[pid2idx(pid)].status == PROC_UNUSED); // pid must be exited
    int pididx = pid2idx(pid);
    if(proc_set[pididx].status != PROC_UNUSED) return;
    /* [lab3-ex1]
     * TODO: collect pid's scheduling information */

    // unsigned long long current_time = earth->gettime();
    // unsigned long long cur_ofcount = earth->overflow_count;
    proc_set[pididx].turnaround_time = time_gap(earth->gettime(), proc_set[pididx].arrive_time, earth->overflow_count, proc_set[pididx].arrive_ofcount);
    //proc_set[pid2idx(pid)].turnaround_time = time_gap(current_time, proc_set[pid2idx(pid)].arrive_time);



    INFO("proc %d died after %d yields, turnaround time: %.4f, response time: %.4f, cputime: %.4f",
            pid, yield_num(pid),
            tar_time(pid), resp_time(pid),
            cpu_runtime(pid));

#ifdef MLFQ
    /* [lab3-ex2]
     * TODO: remove process from queues */
    //printf("status: %d\n", proc_set[pid2idx(pid)].status);
        if(try_rm_item(&hq, (void*)pid)) return;
        if(try_rm_item(&mq, (void*)pid)) return;
        if(try_rm_item(&lq, (void*)pid)) return;
    //try_rm_item(&hq, (void*)pid);
    //try_rm_item(&mq, (void*)pid);
    //try_rm_item(&lq, (void*)pid);
    // INFO("not find %d\n", pid);


#endif
}

/* [lab3-ex3]
 * let pid sleep for the given time units (i.e.., time_units of QUANTUM):
 *   - set pid's status to PROC_SLEEPING
 *   - record some information in "schd_attr" to later decide when the time is up
 *   - later (whenever scheduling is happening), check if the process should be waked up
 *   - in your scheduler (mlfq), if no process can run but there exists sleeping processes,
 *     the scheduler should not return; it should busy loop until one of the
 *     sleeping processes times up.
 *
 * hints:
 *   - you will likely modify multiple other places in your MLFQ implementation
 *   - you probably want a new sleeping queue
 *     -- if so, remember to remove this process from its original queue
 * */
void proc_on_sleep(int pid, int time_units) {
    // ASSERTX(proc_set[pid2idx(pid)].status != PROC_SLEEPING);

    /* TODO: your code here */
    int pididx = pid2idx(pid);
    proc_set[pididx].status = PROC_SLEEPING;
    proc_set[pididx].wake_up_ovc = earth->overflow_count;
    unsigned long long cur_time = earth->gettime();
    proc_set[pididx].wake_up_time = cur_time + time_units * QUANTUM;
    if(cur_time > proc_set[pididx].wake_up_time) {
        ++proc_set[pididx].wake_up_ovc;
    }


}

