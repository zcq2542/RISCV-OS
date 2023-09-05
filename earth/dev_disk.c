/*
 * (C) 2022, Cornell University
 * All rights reserved.
 */

/* Author: Yunhao Zhang
 * Description: a simple disk device driver
 *
 * updated by CS6640 23fall staff
 */

#include "egos.h"
#include "disk.h"
#include "bus_gpio.c"
#include <string.h>

enum {
      FLASH_ROM
};
static int type;

int disk_read(int block_no, int nblocks, char* dst) {
    if (type == FLASH_ROM) {
        char* src = (char*)0x20800000 + block_no * BLOCK_SIZE;
        memcpy(dst, src, nblocks * BLOCK_SIZE);
    } else {
        ASSERT(0, "disk type is unknown");
    }
    return 0;
}

int disk_write(int block_no, int nblocks, char* src) {
    if (type == FLASH_ROM) {
        FATAL("Attempt to write the on-board ROM");
    } else {
        ASSERT(0, "write to disk type unknown");
    }
    return 0;
}

void disk_init() {
    earth->disk_read = disk_read;
    earth->disk_write = disk_write;

    /* QEMU only uses the on-board ROM as disk; */
    // FIXME: add disk
    type = FLASH_ROM;
    return;
}
