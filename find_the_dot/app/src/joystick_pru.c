#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <pthread.h>
#include <string.h>
#include "hal/general_command.h"
#include "../pru-as4/Linux/sharedDataStruct.h"
// General PRU Memomry Sharing Routine
// ----------------------------------------------------------------
#define PRU_ADDR 0x4A300000 // Start of PRU memory Page 184 am335x TRM
#define PRU_LEN 0x80000 // Length of PRU memory
#define PRU0_DRAM 0x00000 // Offset to DRAM
#define PRU1_DRAM 0x02000
#define PRU_SHAREDMEM 0x10000 // Offset to shared memory
#define PRU_MEM_RESERVED 0x200 // Amount used by stack and heap
// Convert base address to each memory section
#define PRU0_MEM_FROM_BASE(base) ( (base) + PRU0_DRAM + PRU_MEM_RESERVED)
#define PRU1_MEM_FROM_BASE(base) ( (base) + PRU1_DRAM + PRU_MEM_RESERVED)
#define PRUSHARED_MEM_FROM_BASE(base) ( (base) + PRU_SHAREDMEM)
// Return the address of the PRU's base memory

static bool shutdown = false;
static pthread_t joystickThread;

static bool joystickRight = true;
static bool joystickDown = true;

//returns true if the joystick right is pressed
bool isJoystickRightPressed() {
    return !joystickRight;
}

//returns true if the joystick down is pressed
bool isJoystickDownPressed() {
    return !joystickDown;
}


static volatile void* getPruMmapAddr(void) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) {
        perror("ERROR: could not open /dev/mem");
        exit(EXIT_FAILURE);
    }
    // Points to start of PRU memory.
    volatile void* pPruBase = mmap(0, PRU_LEN, PROT_READ | PROT_WRITE,
    MAP_SHARED, fd, PRU_ADDR);
    if (pPruBase == MAP_FAILED) {
        perror("ERROR: could not map memory");
        exit(EXIT_FAILURE);
    }
    close(fd);
    return pPruBase;
}
static void freePruMmapAddr(volatile void* pPruBase){
    if (munmap((void*) pPruBase, PRU_LEN)) {
        perror("PRU munmap failed");
        exit(EXIT_FAILURE);
    }
}

void *joystick(void *args) {
    (void)args;
    volatile void *pPruBase = getPruMmapAddr();
    volatile sharedMemStruct_t *pSharedPru0 = PRU0_MEM_FROM_BASE(pPruBase);

    while(!shutdown) {
        joystickRight = pSharedPru0->isJoyStickRightPressed;
        joystickDown = pSharedPru0->isjoyStickDownPressed;
    }
    freePruMmapAddr(pPruBase);
    return NULL;
}


void joystickInit() {
    runCommand("config-pin P8.15 pruin");
    runCommand("config-pin P8.16 pruin");
    pthread_create(&joystickThread, NULL, joystick, NULL);
}

void joystickShutdown() {
    shutdown = true;
    pthread_join(joystickThread, NULL);
}