/*
 * a program corrupting the memory of earth, disk, and free memory.
 */

#include "app.h"
#include <string.h>

void tamper_proc() {
    printf("before: %d\n", *(int*)(FREE_MEM_END - PAGE_SIZE));
    memset((void*)(FREE_MEM_END - PAGE_SIZE), 0, PAGE_SIZE);
    printf("after: %d\n", *(int*)(FREE_MEM_END - PAGE_SIZE));

    /* [Lab4-ex5]
     * TODO: uncomment the following line
     * and answer the questions in slack/lab4.txt
     */
    // memset((void*)FREE_MEM_START, 0, PAGE_SIZE);

    SUCCESS("Crash3 succeeds in corrupting the memory of other processes");
}

void tamper_earth() {
    /* modifying earth's code */
    /* illegal instruction test. 
    unsigned int pmpaddr1;
    asm volatile("csrr %0, pmpaddr1" : "=r"(pmpaddr1));
    printf("pmpaddr1: %x\n", pmpaddr1);
    */
    int *tmp = (int*)0x20400000;
    printf("*tmp: %d\n", *tmp);
    printf("tmp: %x\n", tmp);

    *tmp = 0;
    printf("*tmp: %d\n", *tmp);
    *(int*)(0x20400000) = 0;
    printf("*(int*)0x20400000: %d\n", *(int*)0x20400000);
    *(int*)(0x20400000) = 8389267;
    printf("*(int*)0x20400000: %d\n", *(int*)0x20400000);
    SUCCESS("Crash3 succeeds in modifying earth's code");
}

void tamper_disk() {
    /* modifying disk content*/
    int *tmp = (int*)0x20800000;
    printf("*tmp: %d\n", *tmp);
    printf("tmp: %x\n", tmp);

    *tmp = 1;
    printf("*tmp: %d\n", *tmp);
    *tmp = 0;
    printf("*tmp: %d\n", *tmp);

    SUCCESS("Crash3 succeeds in modifying disk contents");
}

int main(int argc, char **argv) {
    if (argc == 1) {
        tamper_earth();
        tamper_disk();
        tamper_proc();
    } else if (strcmp(argv[1], "disk") == 0) {
        tamper_disk();
    } else if (strcmp(argv[1], "earth") == 0) {
        tamper_earth();
    } else if (strcmp(argv[1], "proc") == 0) {
        tamper_proc();
    } else {
        printf("Usage: crash3 [proc|disk|earth]\n");
    }
}
