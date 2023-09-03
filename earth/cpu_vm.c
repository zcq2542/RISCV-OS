/* Description: Virtual Memory Management*/

#include "egos.h"
#include "string.h"

/* temporary helper functions from cpu_mmu.c
 * should be removed after PTs are ready
 */
int soft_tlb_map(int pid, void *dst, void *src);
int soft_tlb_switch(int pid);
int soft_tlb_free(int pid);

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


#define MAX_ROOT_PAGE_TABLES MAX_NPROCESS

/* a mapping from pid to page table root */
static m_uint32* pid_to_pagetable_base[MAX_ROOT_PAGE_TABLES];

m_uint32* walk(int pid, void* va, int alloc) {
    /* TODO: your code here */

}


int page_table_map(int pid, void *va, void *pa) {
    if (pid >= MAX_ROOT_PAGE_TABLES) FATAL("page_table_map: pid too large");
    soft_tlb_map(pid, va, pa);
}

int page_table_free(int pid) {
    soft_tlb_free(pid);
}


void *page_table_translate(int pid, void *va) {
}

int page_table_switch(int pid) {
    /* ensure all updates to page table are settled */
    asm("sfence.vma zero,zero");

    soft_tlb_switch(pid);

    /* wait flushing TLB entries */
    asm("sfence.vma zero,zero");
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

    /* Allocate the L2 page table page */
    m_uint32* l2_pa = pmalloc(1);
    memset(l2_pa, 0, PAGE_SIZE);

    /* Setup the entry in the root page table */
    int vpn1 = addr >> 22;
    m_uint32 ppn = ((m_uint32)l2_pa >> 12);
    root[vpn1] =  (ppn << 10) | FLAG_NEXT_LEVEL;

    /* Setup the PTE in the l2 page table page */
    int vpn0 = (addr >> 12) & 0x3FF;
    for (int i = 0; i < npages; i++) {
        l2_pa[vpn0 + i] = ((addr + i * PAGE_SIZE) >> 2) | FLAG_VALID_RWX;
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
    setup_identity_region(0, 0x20400000, 1024); /* boot ROM */
    setup_identity_region(0, 0x20800000, 1024); /* disk image */
    setup_identity_region(0, 0x80000000, 1024); /* DTIM memory */

    for (int i = 0; i < 8; i++) {                 /* ITIM memory is 32MB on QEMU */
        setup_identity_region(0, 0x08000000 + i * 0x00400000, 1024);
    }
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
