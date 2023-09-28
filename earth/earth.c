/*
 * (C) 2022, Cornell University
 * All rights reserved.
 */

/* Author: Yunhao Zhang
 * Description: Initialize the bss and data segments;
 * Initialize dev_tty, dev_disk, cpu_intr and cpu_mmu;
 * Load the grass layer binary from disk and run it.
 *
 * updated by CS6640 23fall staff
 */

#include "elf.h"
#include "disk.h"
#include "egos.h"
#include <string.h>

void tty_init();
void disk_init();
void intr_init();
void timer_init();
void mmu_init();

struct grass *grass = (void*)APPS_STACK_TOP;
struct earth *earth = (void*)GRASS_STACK_TOP;
extern char bss_start, bss_end, data_rom, data_start, data_end;

static void earth_init() {
    tty_init();
    CRITICAL("------------- Booting -------------");
    SUCCESS("Finished initializing the tty device");

    intr_init();
    SUCCESS("Finished initializing the CPU interrupts");

    timer_init();
    SUCCESS("Finished initializing the timer function");

    disk_init();
    SUCCESS("Finished initializing the disk device");

    mmu_init();
    SUCCESS("Finished initializing the CPU memory management unit");
}

static int grass_read(int block_no, char* dst) {
    return earth->disk_read(GRASS_EXEC_START + block_no, 1, dst);
}

int main() {
    /* Prepare the bss and data memory regions */
    memset(&bss_start, 0, (&bss_end - &bss_start));
    memcpy(&data_start, &data_rom, (&data_end - &data_start));

    /* Initialize the earth layer */
    earth_init();

    /* Load and enter the grass layer */
    elf_load(0, grass_read, 0, 0);
    void (*grass_entry)() = (void*)GRASS_ENTRY;
    grass_entry();
}
