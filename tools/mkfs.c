/*
 * (C) 2022, Cornell University
 * All rights reserved.
 */

/* Author: Yunhao Zhang
 * Description: create the disk image file (disk.img)
 * The disk image should be exactly 4MB:
 *     the first 1MB is reserved as 256 frames for memory paging;
 *     the next  1MB contains some ELF binary executables for booting;
 *     the last  2MB is managed by a file system.
 * The output is in binary format (disk.img).
 *
 * Updated by CS6640 23fall staff
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "disk.h"
#include "file.h"

#define NKERNEL_PROC 1
char* kernel_processes[] = {
                            "../build/release/grass.elf",
};


char fs[FS_DISK_SIZE], exec[GRASS_EXEC_SIZE];

inode_intf ramdisk_init();

int main() {
    /* Paging area */
    freopen("disk.img", "w", stdout);
    write(1, exec, PAGING_DEV_SIZE); // 1MB buffer

    /* Grass kernel processes */
    int exec_size = GRASS_EXEC_SIZE / GRASS_NEXEC;
    fprintf(stderr, "[INFO] Loading %d kernel binary files\n", NKERNEL_PROC);

    for (int i = 0; i < NKERNEL_PROC; i++) {
        struct stat st;
        stat(kernel_processes[i], &st);
        assert((st.st_size > 0) && (st.st_size <= exec_size));
        fprintf(stderr, "[INFO] Loading %s: %ld bytes\n", kernel_processes[i], (long)st.st_size);

        // open kernel process elf and use stdin (0) as file descriptor
        freopen(kernel_processes[i], "r", stdin);
        memset(exec, 0, GRASS_EXEC_SIZE);

        // read kernel process elf to the buffer "exec"
        for (int nread = 0; nread < st.st_size; )
            nread += read(0, exec + nread, exec_size - nread);

        // write kernel process elf to disk.img
        write(1, exec, st.st_size);

        // write kernel process elf again and fill in the gap
        write(1, &exec[st.st_size], exec_size - st.st_size);
    }

    // fill in empty for the remaining 1MB space
    memset(exec, 0, GRASS_EXEC_SIZE);
    write(1, exec, (GRASS_NEXEC - NKERNEL_PROC) * exec_size);

    /* File system */
    write(1, fs, FS_DISK_SIZE);
    fclose(stdout);

    fprintf(stderr, "[INFO] Finish making the disk image (tools/disk.img)\n");
    return 0;
}


int getsize() { return FS_DISK_SIZE / BLOCK_SIZE; }

int setsize() { assert(0); }

int ramread(inode_intf bs, unsigned int ino, block_no offset, block_t *block) {
    memcpy(block, fs + offset * BLOCK_SIZE, BLOCK_SIZE);
    return 0;
}

int ramwrite(inode_intf bs, unsigned int ino, block_no offset, block_t *block) {
    memcpy(fs + offset * BLOCK_SIZE, block, BLOCK_SIZE);
    return 0;
}

inode_intf ramdisk_init() {
    inode_store_t *ramdisk = malloc(sizeof(*ramdisk));

    ramdisk->read = (void*)ramread;
    ramdisk->write = (void*)ramwrite;
    ramdisk->getsize = (void*)getsize;
    ramdisk->setsize = (void*)setsize;

    return ramdisk;
}

