/*
 * (C) 2022, Cornell University
 * All rights reserved.
 */

/* Author: Yunhao Zhang
 * Description: memory management unit (MMU)
 *
 * updated by CS6640 23fall staff
 */

#include "egos.h"
#include <string.h>

/* Interface to allocate/free a physical page */
void* pmalloc(int clear);
void  pfree(void *paddr);
void *pageid2paddr(int pageid);

typedef struct {
    void *private_addr;
    void *exec_addr;
} page_mapping;

// number of pages of an app
#define APPS_NPAGES (APPS_SIZE/PAGE_SIZE + 2)
page_mapping app_pages[MAX_NPROCESS][APPS_NPAGES];

void* mmu_alloc() {
    return  pmalloc(1);
}

int soft_tlb_free(int pid) {
    ASSERT(pid > 0 && pid<MAX_NPROCESS, "soft_tlb_free: pid is unexpected");
    page_mapping *pages = app_pages[pid];
    for (int i = 0; i < APPS_NPAGES; i++) {
        if (pages[i].private_addr != NULL) {
            pfree(pages[i].private_addr);
            pages[i].private_addr = NULL;
            pages[i].exec_addr = NULL;
        }
    }
}

/* Software TLB Translation */
int soft_tlb_map(int pid, void *dst, void *src) {
    ASSERT(pid > 0 && pid<MAX_NPROCESS, "soft_tlb_map: pid is unexpected");
    ASSERT( ((unsigned int)src % PAGE_SIZE == 0) && \
            ((unsigned int)dst % PAGE_SIZE == 0), \
            "soft_tlb_map: mapping address is not page aligned");

    page_mapping *pages = app_pages[pid];
    for (int i = 0; i < APPS_NPAGES; i++) {
        if (pages[i].private_addr == NULL) {
            pages[i].private_addr = src;
            pages[i].exec_addr = dst;
            return 0;
        }
    }

    FATAL("soft_tlb_map: mapping more pages than expected");
}

// a helper function to copy memory
void memcopy_helper(int pid, int private2exec) {
    ASSERT(pid > 0 && pid<MAX_NPROCESS, "memcopy_helper: pid is unexpected");

    page_mapping *pages = app_pages[pid];
    for (int i = 0; i < APPS_NPAGES; i++) {
        if (pages[i].private_addr != NULL) {
            ASSERT(pages[i].exec_addr != NULL, "");

            void *src = (private2exec) ? \
                        pages[i].private_addr : pages[i].exec_addr;
            void *dst = (private2exec) ? \
                        pages[i].exec_addr : pages[i].private_addr;

            memcpy(dst, src, PAGE_SIZE);
        }
    }
}

int soft_tlb_switch(int pid) {
    static int curr_vm_pid = -1;
    if (pid == curr_vm_pid) return 0;

    /* for curr_vm_pid,
     * copy from exec memory back to the private pages*/
    if (curr_vm_pid > 0) {
        memcopy_helper(curr_vm_pid, 0 /*private2exec? no*/);
    }

    /* for pid (new running process),
     * copy from private pages to exec addresses */
    memcopy_helper(pid, 1 /*private2exec? yes*/);

    curr_vm_pid = pid;
}

void pmp_init() {
    /* [lab4-ex4]
     * TODO: set PMP memory protection */
    unsigned int pmpcfg = 0;
    unsigned int pmpaddr = 0;
    /* Setup PMP TOR region 0x00000000 - 0x20000000 as r/w/x */
   
    // Construct the configuration value. This is an example; actual encoding might differ.
    /*
    unsigned int end_address = 0x20000000;  // Shifting not necessary for 0x20000000; illustrative only
    //write_csr(pmpaddr0, end_address);  // Writing the end address to pmpaddr register (e.g., pmpaddr0)
    asm("csrw pmpaddr0, %0" :: "r"(end_address));
    
    const unsigned int PMP_READ  = 0b00000001;
    const unsigned int PMP_WRITE = 0b00000010;
    const unsigned int PMP_EXEC  = 0b00000100;
    const unsigned int PMP_TOR   = 0b00001000;  // Example encoding for TOR    
    const unsigned int PMP_NAPOT = 0b00011000;
    unsigned int cfg_value = PMP_READ | PMP_WRITE | PMP_EXEC | PMP_TOR;  // R/W/X permissions, TOR addressing

    */
    // You may need to shift cfg_value to match the correct entry
    // If configuring the first entry in pmpcfg0, no shift is necessary

    //write_csr(pmpcfg0, cfg_value);  // Writing the configuration for the corresponding pmpaddr

    
 
    asm volatile ("csrr %0, pmpcfg0" : "=r"(pmpcfg));
    printf("init pmpconfig: %x\n", pmpcfg);
    asm volatile ("csrr %0, pmpaddr0" : "=r"(pmpaddr));
    printf("init pmpaddr0: %x\n", pmpaddr);

    //pmpcfg |= (1 << 7)|(1 << 3)|(1 << 2)|(1 << 1)|(1 << 0);
    
    pmpcfg |= (1 << 3)|(1 << 2)|(1 << 1)|(1 << 0);
    pmpaddr = 0x20000000 >> 2;
    printf("pmpconfig: %x\n", pmpcfg);
    //asm volatile ("csrw pmpcfg0, %0" :: "r"(pmpcfg));
    asm volatile ("csrw pmpaddr0, %0" :: "r"(pmpaddr));
    printf("pmpaddr0: %x\n", pmpaddr);
    
    /* Setup PMP NAPOT region 0x20400000 - 0x20800000 as r/-/x */
    /*
    cfg_value |= (PMP_READ | PMP_EXEC | PMP_NAPOT) << 8;
    printf("cfg_value: %x\n", cfg_value);
    unsigned int pmpaddr1 = (0x20400000 >> 2) + (0x400000 >> 3) -1;
    printf("pmpaddr1: %x\n", pmpaddr1);

    asm("csrw pmpcfg0, %0" :: "r"(cfg_value));
    asm("csrw pmpaddr1, %0" :: "r"(pmpaddr1));
    */
    //pmpcfg |= (((1 << 7)|(1 << 3)|(1 << 2)|(1 <<0)) << 16);
    /*
    pmpcfg |= (((0 << 2)|(0 <<0)) << 8);
    pmpcfg |= (((0 << 3)|(0 << 2)|(0 <<0)) << 16);
    printf("pmpconfig: %x\n", pmpcfg);
    asm volatile("csrw pmpcfg0, %0" :: "r"(pmpcfg));
    pmpaddr = 0x20400000 >> 2;
    asm volatile("csrw pmpaddr1, %0" :: "r"(pmpaddr));
    pmpaddr = 0x20800000 >> 2;
    asm volatile("csrw pmpaddr2, %0" :: "r"(pmpaddr));
    */

   
    pmpcfg |= (((3 << 3)|(1 << 2)|(1 <<0)) << 8);
    printf("pmpconfig: %x\n", pmpcfg);
    //asm("csrw pmpcfg0, %0" :: "r"(pmpcfg));
    unsigned int size = 0x400000;
    unsigned int mask = size/2 - 1;
    //pmpaddr = (0x20400000 + mask) >> 2; 
    printf("%x\n", (0x20400000) >> 2);
    printf("%x\n", (size >> 3) - 1);
    pmpaddr = ((0x20400000) >> 2) + (size >> 3) - 1; 
    asm("csrw pmpaddr1, %0" :: "r"(pmpaddr));
    printf("pmpaddr1: %x\n", pmpaddr);
    

    /* Setup PMP NAPOT region 0x20800000 - 0x20C00000 as r/-/- */
    
    pmpcfg |= (((3 << 3)|(1 << 0)) << 16);
    printf("pmpconfig: %x\n", pmpcfg);
    //asm("csrw pmpcfg0, %0" :: "r"(pmpcfg));
    pmpaddr = (0x20800000 + mask) >> 2;
    asm("csrw pmpaddr2, %0" :: "r"(pmpaddr));
    printf("pmpaddr2: %x\n", pmpaddr);

    /* Setup PMP NAPOT region 0x80000000 - 0x80400000 as r/w/- */
   
    pmpcfg |= (((3 << 3)|(1 << 1)|(1 << 0)) << 24);
    //asm("csrw pmpcfg0, %0" :: "r"(pmpcfg));
    pmpaddr = (0x80000000 + mask) >> 2;
    asm("csrw pmpaddr3, %0" :: "r"(pmpaddr));
    printf("pmpconfig: %x\n", pmpcfg);
    printf("pmpaddr3: %x\n", pmpaddr);

    asm volatile ("csrw pmpcfg0, %0" :: "r"(pmpcfg));

    asm volatile ("csrr %0, pmpcfg0" : "=r"(pmpcfg));
    printf("read pmpconfig0: %x\n", pmpcfg);
    unsigned int pmpaddr0 = 0;
    asm volatile ("csrr %0, pmpaddr0" : "=r"(pmpaddr0));
    printf("reaD pmpaddr0: %x\n", pmpaddr0);
    
    asm volatile ("csrr %0, pmpaddr1" : "=r"(pmpaddr0));
    printf("reaD pmpaddr1: %x\n", pmpaddr0);

    asm volatile ("csrr %0, pmpaddr2" : "=r"(pmpaddr0));
    printf("reaD pmpaddr2: %x\n", pmpaddr0);

    asm volatile ("csrr %0, pmpaddr3" : "=r"(pmpaddr0));
    printf("reaD pmpaddr3: %x\n", pmpaddr0);
}

/* defined in cpu_vm.c */
void vm_init();

/* MMU Initialization */
void mmu_init() {
    /* Initialize MMU interface functions */
    earth->mmu_alloc = mmu_alloc;
    earth->mmu_free = soft_tlb_free;
    earth->mmu_map = soft_tlb_map;
    earth->mmu_switch = soft_tlb_switch;

    /* Initialize memory protection */
    pmp_init();

    /* Initialize virtual memory */
    //vm_init();
}
