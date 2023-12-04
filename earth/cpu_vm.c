/* Description: Virtual Memory Management*/

#include "egos.h"
#include "string.h"

/* Page Table Translation
 *
 * The code below creates an identity mapping using RISC-V Sv32;
 * Read section4.3 of RISC-V privileged spec manual (riscv-privileged-v1.10.pdf)
 */

#define FLAG_VALID_RWX 0xF
#define FLAG_NEXT_LEVEL 0x1

/* Interface to allocate/free a physical page */
void* pmalloc(int clear_page);
void  pfree(void *paddr);
void bitmap_print();

#define MAX_ROOT_PAGE_TABLES MAX_NPROCESS
#define PA2PTE(PA) ((PA >> 12) << 10)
#define PTE2PA(PTE) ((PTE >> 10) << 12)

/* a mapping from pid to page table root */
static m_uint32* pid_to_pagetable_base[MAX_ROOT_PAGE_TABLES];

void fence() {
    asm("sfence.vma zero,zero");
}
void sysproc_identity_mapping(int pid);
void sysproc_identity_unmapping(int pid);
void userproc_identity_mapping(int pid);
void userproc_identity_unmapping(int pid); 
        
/* [lab5-ex1]:
 * this function walks the page table:
 *   it translates virtual address "va" via page table "root",
 *   and returns the **address** of the PTE pointing to "va".
 * if alloc is non-zero,
 *   allocate the pages when necessary.
 * if alloc is zero,
 *   FATAL when cannot find the page.
 *
 * hints:
 *  - the return value is a PTE pointer.
 *    With the pointer, it is easy to change the permissions on PTE.
 *  - use pmalloc() to allocate pages
 */
m_uint32* walk(m_uint32* root, void* va, int alloc) {
    /* TODO: your code here */
    m_uint32 l1Idx = (m_uint32)va >> 22;
    //m_uint32 l2Idx = (m_uint32)va >> 12 & (~(l1Idx << 10));
    m_uint32 l2Idx = ((m_uint32)va << 10) >> 22;
    //m_uint32 *l1_pte_ptr = root + l1Idx * 4; // pointer calculation wrong.
    //m_uint32 *l1_pte_ptr = root + l1Idx;
    m_uint32 *l1_pte_ptr = &root[l1Idx];
    m_uint32 l1_pte = *(l1_pte_ptr);
    m_uint32 *l2_pagetable = NULL;
    m_uint32 *l2_pte_ptr = NULL;
    //if((l1_pte & FLAG_VALID_RWX) == FLAG_NEXT_LEVEL){ // X,R,W,V = 0001
    if(l1_pte & FLAG_NEXT_LEVEL){ // X,R,W,V = 0001
        //printf("");
        l2_pagetable = (m_uint32*)((l1_pte >> 10) << 12);
        
    }
    else{
        if(!alloc){
            FATAL("invalid pte");
        }
        else{
            printf("going to maclloc l2_pagetable\n");
            l2_pagetable = (m_uint32*)pmalloc(1); // malloc a page
            *l1_pte_ptr = (((m_uint32)l2_pagetable >> 12) << 10) | FLAG_NEXT_LEVEL; //update l1_pte
            printf("*l1_pte_ptr: %x\n", *l1_pte_ptr);
        }
    }
    l2_pte_ptr = &l2_pagetable[l2Idx];
    //printf("*l2_pte_ptr: %x\n", *l2_pte_ptr);
    //*l2_pte_ptr = 0x1;
    return l2_pte_ptr;
}

void print_pagetable(m_uint32* root){
    if(root == NULL){
        printf("root is NULL\n");
    }
    else{
        //printf("pagetable addr: %x\n", root);
        for(int i = 0; i < 1024; ++i){
            if(root[i] != 0){
                printf("\troot[%d]: %x\n", i, root[i]);
                m_uint32* l2_pa = (m_uint32*)((root[i] >> 10) << 12);
                for(int j = 0; j < 1024; ++j){
                    if(l2_pa[j] != 0){
                        printf("\t\tl2_pa[%d]: %x.\t pa: %x\t", j, l2_pa[j], (l2_pa[j] >> 10) << 12);
                        printf("va: %x\n", (i << 22) + (j << 12));
                        
                    }
                }
            }
        }
    }
    printf("\n");
}

/* [lab5-ex1]
 * establish the mapping between the "va" to the "pa" for process "pid"
 */
int page_table_map(int pid, void *va, void *pa) {
    ASSERT(pid >= 0 && pid <= MAX_NPROCESS, "pid is unexpected");
    ASSERT( ((m_uint32)va % PAGE_SIZE == 0) &&
            ((m_uint32)pa % PAGE_SIZE == 0), "page is not aligned");

    //printf("pid: %d\tpa: %x\tva: %x\n", pid, pa, va);
    m_uint32 *root = pid_to_pagetable_base[pid];

    /* Implement va-pa mapping:
     * (1) if the page table for pid does not exist, build the page table.
     *     (a) if the process is a system process (pid < USER_PID_START)
     *       you need to map the well-known memory to system process address space.
     *       Here are the well-known memory regions:
     *           | start address | #pages| explanation
     *           +---------------+-------+------------------
     *           |  0x02000000   | 16    | CLINT
     *           |  0x08000000   | 512   | earth data, grass code+data
     *           |  0x10013000   | 1     | UART0
     *           |  0x20400000   | 256   | earth code
     *           |  0x20800000   | 1024  | disk data
     *           |  0x80000000   | 1024  | DTIM memory
     *       hint: you should use "setup_identity_region()"
     *     (b) if the process is a user process (pid >= USER_PID_START)
     *       you need to map the following well-known memory to the user address space.
     *           | start address | #pages| explanation
     *           +---------------+-------+------------------
     *           |  0x08000000   | 512   | earth data, grass code+data
     *           |  0x10013000   | 1     | UART0
     *           |  0x20400000   | 256   | earth code
     *           |  0x80002000   | 2     | grass interface
     *           |               |       | + earth/grass stack
     *           |               |       | + earth interface
     *
     * (2) if the page tables exists,
     *   use walk() to get the PTE pointer,
     *   and update PTE accordingly.
     *   note:
     *   system processes run in supervisor mode; others run in user mode.
     *   You should update their PTE permission bits accordingly.
     */

    /* TODO: your code here */
    //FATAL("page_table_map is not implemented.");
    if(root == NULL){
        root = (m_uint32*)pmalloc(1);
        pid_to_pagetable_base[pid] = root;
        if(pid < USER_PID_START){
            sysproc_identity_mapping(pid);
        }
        else{
            userproc_identity_mapping(pid);        
        }
    }
    
    m_uint32* pte_ptr =  walk(root, va, 1);
    *(pte_ptr) = (((m_uint32)pa >> 12) << 10) | FLAG_VALID_RWX;
    //printf("*pte_ptr: %x\n", *pte_ptr);
    if(pid < USER_PID_START){
        //*(pte_ptr) |= 0x10; // S
    }
    else{
        *(pte_ptr) |= 0x1F; // U
    }
//    print_pagetable(root); 
    //m_uint32* test_pte_ptr = walk(root, va, 0);
    //printf("*test_pte_ptr: %x\n", *test_pte_ptr);
    /* wait flushing TLB entries */
   fence();
}


/* [lab5-ex2]
 * switching address space to process "pid"
 * hints:
 * - you will use "asm" to manipulate CSR "satp"
 */
int page_table_switch(int pid) {
    ASSERT(pid >= 0 && pid <= MAX_NPROCESS, "pid is unexpected");
    /* ensure all updates to page table are settled */
    fence();

    /* TODO: your code here */
    //FATAL("page_table_switch is not implemented.");
    m_uint32* root = pid_to_pagetable_base[pid];
    
    //printf("%x\n", root);
    //m_uint32 before = 0;
    //asm volatile("csrr %0, satp" :"=r"(before));
    //printf("%x\n", before);
    m_uint32 now = ((m_uint32)root >> 12) | (1 << 31);
    //printf("%x\n", now);
    asm volatile("csrw satp, %0" : : "r"(now));

    /* wait flushing TLB entries */
    fence();
}



/* [lab5-ex3]
 * this function translates the virtual address "va" to its physical address,
 * and returns the physical address.
 */
void *page_table_translate(int pid, void *va) {

    /* TODO: your code here */
    //FATAL("page_table_translate is not implemented.");
    //printf("translate pid: %d, va: %x\n", pid, va);
    m_uint32* pagetable = pid_to_pagetable_base[pid];
    if(pagetable == NULL) FATAL("pid: %d, pagetable is NULL", pid);
    m_uint32* l2_pte_ptr = walk(pagetable, va, 0);
    //printf("l2_pte_ptr: %x\n", l2_pte_ptr);
    //printf("*l2_pte_ptr: %x\n", *l2_pte_ptr);
    //m_uint32 pa = PTE2PA(*l2_pte_ptr);
    m_uint32 pa = (((*l2_pte_ptr) >> 10) << 12) + (((m_uint32)va << 20) >> 20);
    //printf("pa: %x\n", pa);
    return (m_uint32*)pa;
}


/* [lab5-ex4]
 * free page table and all relevant pages
 */
int page_table_free(int pid) {
    ASSERT(pid >= 0 && pid <= MAX_NPROCESS, "pid is unexpected");
    /* To free the page table:
     * (1) free all pages pointed by the page table root
     *     by calling pfree()
     *     (a) loop L1 pte
     *       (b) loop L2 pte
     *         (c) free data page
     *       (b) free L2 page table page
     *     (a) free L1 page table page (pointed by root)
     * (2) set pid_to_pagetable_base[pid] to 0
     *
     * Note:
     * - use pfree() to free pages
     * - take care of the well-known pages (don't free the pages that are not
     *   allocated by your code)
     */

    //FATAL("page_table_free is not implemented.");
    /* TODO: your code here */

    //bitmap_print();
    //printf("\tfree pagetable\n");
    
    if(pid < USER_PID_START){
        sysproc_identity_unmapping(pid);
    } 
    else{
        userproc_identity_unmapping(pid);
    }
    
    m_uint32* root = pid_to_pagetable_base[pid];
    for(int i = 0; i < 1024; ++i){
        if(root[i] != 0){
            m_uint32* l2_pa = (m_uint32*)PTE2PA(root[i]); 
            for(int j = 0; j < 1024; ++j){
                if(l2_pa[j] != 0){
                    m_uint32* pa = (m_uint32*)PTE2PA(l2_pa[j]);
                    pfree(pa); // free data page
                    l2_pa[j] = 0; 
                }
            }
            pfree(l2_pa); // free L2 page table page
        }
    }
    pfree(root); // free L1 page table page (pointed by root)
    pid_to_pagetable_base[pid] = NULL;
    /* wait flushing TLB entries */
    fence();
    //bitmap_print();
}


/* set up virtual memory mapping for process pid,
 * by mapping
 *      VA [addr, addr + npages*PAGE_SIZE)
 * to
 *      PA [addr, addr + npages*PAGE_SIZE)
 * */
void setup_identity_region(int pid, m_uint32 addr, int npages) {
    ASSERT(npages <= 1024, "npages is larger than 1024");
    ASSERT((addr & 0xFFF) == 0, "addr is not 4K aligned");

    m_uint32* root = pid_to_pagetable_base[pid];
    ASSERT(root != NULL, "pagetable root is NULL");

    /* Setup the entry in the root page table */
    m_uint32* l2_pa;
    int vpn1 = addr >> 22;
    if (root[vpn1] & FLAG_NEXT_LEVEL) { // l2 PT page exists
        l2_pa = (m_uint32*)  ((root[vpn1] >> 10) << 12);
    } else {
        /* Allocate the L2 page table page */
        l2_pa = pmalloc(1);
        m_uint32 ppn = ((m_uint32)l2_pa >> 12);
        root[vpn1] =  (ppn << 10) | FLAG_NEXT_LEVEL;
    }

    /* Setup the PTE in the l2 page table page */
    int vpn0 = (addr >> 12) & 0x3FF;
    for (int i = 0; i < npages; i++) {
        ASSERT(l2_pa[vpn0 + i] == 0, "non-empty l2 PTE");
        /* [lab5-ex3]
         * TODO: for user processes, set PTE_U */
        l2_pa[vpn0 + i] = ((addr + i * PAGE_SIZE) >> 2) | FLAG_VALID_RWX;
        
        if(pid >= USER_PID_START){
            l2_pa[vpn0 + i] |= 0x10; 
        }
        
    }
}

void unmapping_identity_region(int pid, m_uint32 addr, int npages) {
    
    ASSERT(npages <= 1024, "npages is larger than 1024");
    ASSERT((addr & 0xFFF) == 0, "addr is not 4K aligned");

    m_uint32* root = pid_to_pagetable_base[pid];
    ASSERT(root != NULL, "pagetable root is NULL");

    /* Allocate the L2 page table page */
    for (int i = 0; i < npages; i++) {
        m_uint32* l2_pte_ptr = walk(root, (m_uint32*)(addr + i * PAGE_SIZE), 0);
        *l2_pte_ptr = 0;
    }
}

void kernel_identity_mapping() {
    /* Allocate kernel's page table.
     * It maps kernel's VA to the identical PA. */
    void *kernel_root = pmalloc(1);
    pid_to_pagetable_base[0] = (m_uint32*)kernel_root;

    /* Allocate the leaf page tables */
    setup_identity_region(0, 0x02000000, 16);   /* CLINT */
    setup_identity_region(0, 0x10013000, 1);    /* UART0 */
    setup_identity_region(0, 0x10024000, 1);    /* SD card */
    setup_identity_region(0, 0x20400000, 1024); /* boot ROM */
    setup_identity_region(0, 0x20800000, 1024); /* disk image */
    setup_identity_region(0, 0x80000000, 1024); /* DTIM memory */

    for (int i = 0; i < 8; i++) {                 /* ITIM memory is 32MB on QEMU */
        setup_identity_region(0, 0x08000000 + i * 0x00400000, 1024);
    }
    fence();
}

void sysproc_identity_mapping(int pid) {
    /* Allocate sysproc's page table.
     * It maps sysproc's VA to the identical PA. 
                 | start address | #pages| explanation
     *           +---------------+-------+------------------
     *           |  0x02000000   | 16    | CLINT
     *           |  0x08000000   | 512   | earth data, grass code+data
     *           |  0x10013000   | 1     | UART0
     *           |  0x20400000   | 256   | earth code
     *           |  0x20800000   | 1024  | disk data
     *           |  0x80000000   | 1024  | DTIM memory
     */
    /* Allocate the leaf page tables */
    setup_identity_region(pid, 0x02000000, 16);   /* CLINT */
    setup_identity_region(pid, 0x10013000, 1);    /* UART0 */
    setup_identity_region(pid, 0x10024000, 1);    /* SD card */
    setup_identity_region(pid, 0x20400000, 256); /* boot ROM */
    setup_identity_region(pid, 0x20800000, 1024); /* disk image */
    setup_identity_region(pid, 0x80000000, 1024); /* DTIM memory */

    setup_identity_region(pid, 0x08000000, 512);
    fence();
}

void sysproc_identity_unmapping(int pid) {
    /* Allocate sysproc's page table.
     * It maps sysproc's VA to the identical PA. 
                 | start address | #pages| explanation
     *           +---------------+-------+------------------
     *           |  0x02000000   | 16    | CLINT
     *           |  0x08000000   | 512   | earth data, grass code+data
     *           |  0x10013000   | 1     | UART0
     *           |  0x20400000   | 256   | earth code
     *           |  0x20800000   | 1024  | disk data
     *           |  0x80000000   | 1024  | DTIM memory
     */
    /* Allocate the leaf page tables */
    unmapping_identity_region(pid, 0x02000000, 16);   /* CLINT */
    unmapping_identity_region(pid, 0x10013000, 1);    /* UART0 */
    unmapping_identity_region(pid, 0x10024000, 1);    /* SD card */
    unmapping_identity_region(pid, 0x20400000, 256); /* boot ROM */
    unmapping_identity_region(pid, 0x20800000, 1024); /* disk image */
    unmapping_identity_region(pid, 0x80000000, 1024); /* DTIM memory */

    unmapping_identity_region(pid, 0x08000000, 512);
    fence();
}

void userproc_identity_mapping(int pid) {
    /* Allocate sysproc's page table.
     * It maps sysproc's VA to the identical PA. 
     *        | start address | #pages| explanation
     *           +---------------+-------+------------------
     *           |  0x08000000   | 512   | earth data, grass code+data
     *           |  0x10013000   | 1     | UART0
     *           |  0x20400000   | 256   | earth code
     *           |  0x80002000   | 2     | grass interface
     *           |               |       | + earth/grass stack
     *           |               |       | + earth interface
     */
    /* Allocate the leaf page tables */
    setup_identity_region(pid, 0x10013000, 1);    /* UART0 */
    setup_identity_region(pid, 0x20400000, 256); /* earth code */
    setup_identity_region(pid, 0x80002000, 2); /* ... interface */
    setup_identity_region(pid, 0x08000000, 512); //earth data, grass code+data
    fence();
}

void userproc_identity_unmapping(int pid) {
    /* Allocate sysproc's page table.
     * It maps sysproc's VA to the identical PA. 
     *        | start address | #pages| explanation
     *           +---------------+-------+------------------
     *           |  0x08000000   | 512   | earth data, grass code+data
     *           |  0x10013000   | 1     | UART0
     *           |  0x20400000   | 256   | earth code
     *           |  0x80002000   | 2     | grass interface
     *           |               |       | + earth/grass stack
     *           |               |       | + earth interface
     */
    /* Allocate the leaf page tables */
    unmapping_identity_region(pid, 0x10013000, 1);    /* UART0 */
    unmapping_identity_region(pid, 0x20400000, 256); /* earth code */
    unmapping_identity_region(pid, 0x80002000, 2); /* ... interface */
    unmapping_identity_region(pid, 0x08000000, 512); //earth data, grass code+data
    fence();
}

int vm_init() {
    /* Setup identity mapping for kernel */
    kernel_identity_mapping();

    void *kernel_root = pid_to_pagetable_base[0];
    /* switch to kernel's address space */
    asm("csrw satp, %0" ::"r"(((m_uint32)kernel_root >> 12) | (1 << 31)));

    earth->mmu_map = page_table_map;
    earth->mmu_free = page_table_free;
    earth->mmu_switch = page_table_switch;
    earth->mmu_translate = page_table_translate;
}
