#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <pthread.h>
#include <string.h>
#include "hal/general_command.h"
#include "hal/accelerometer.h"
#include "../pru-as4/Linux/sharedDataStruct.h"
// General PRU Memomry Sharing Routine
// ----------------------------------------------------------------
#define PRU_ADDR 0x4A300000 // Start of PRU memory Page 184 am335x TRM
#define PRU_LEN 0x80000     // Length of PRU memory
#define PRU0_DRAM 0x00000   // Offset to DRAM
#define PRU1_DRAM 0x02000
#define PRU_SHAREDMEM 0x10000  // Offset to shared memory
#define PRU_MEM_RESERVED 0x200 // Amount used by stack and heap
// Convert base address to each memory section
#define PRU0_MEM_FROM_BASE(base) ((base) + PRU0_DRAM + PRU_MEM_RESERVED)
#define PRU1_MEM_FROM_BASE(base) ((base) + PRU1_DRAM + PRU_MEM_RESERVED)
#define PRUSHARED_MEM_FROM_BASE(base) ((base) + PRU_SHAREDMEM)
// Return the address of the PRU's base memory

static bool shutdown = false;
static pthread_t neopixelThread;

uint32_t init_color[STR_LEN] = {
    0x0f000000, // Green
    0x000f0000, // Red
    0x00000f00, // Blue
    0x0000000f, // White
    0x0f0f0f00, // White (via RGB)
    0x0f0f0000, // Yellow
    0x000f0f00, // Purple
    0x0f000f00, // Teal

    // Try these; they are birght!
    // (You'll need to comment out some of the above)
    // 0xff000000, // Green Bright
    // 0x00ff0000, // Red Bright
    // 0x0000ff00, // Blue Bright
    // 0xffffff00, // White
    // 0xff0000ff, // Green Bright w/ Bright White
    // 0x00ff00ff, // Red Bright w/ Bright White
    // 0x0000ffff, // Blue Bright w/ Bright White
    // 0xffffffff, // White w/ Bright White
};

uint32_t off_color[STR_LEN] = {
    0x00000000, // Green
    0x00000000, // Red
    0x00000000, // Blue
    0x00000000, // White
    0x00000000, // White (via RGB)
    0x00000000, // Yellow
    0x00000000, // Purple
    0x00000000, // Teal

    // Try these; they are birght!
    // (You'll need to comment out some of the above)
    // 0xff000000, // Green Bright
    // 0x00ff0000, // Red Bright
    // 0x0000ff00, // Blue Bright
    // 0xffffff00, // White
    // 0xff0000ff, // Green Bright w/ Bright White
    // 0x00ff00ff, // Red Bright w/ Bright White
    // 0x0000ffff, // Blue Bright w/ Bright White
    // 0xffffffff, // White w/ Bright White
};

uint32_t blue_color[STR_LEN] = {
    0x00000f00, // Green
    0x00000f00, // Red
    0x00000f00, // Blue
    0x00000f00, // White
    0x00000f00, // White (via RGB)
    0x00000f00, // Yellow
    0x00000f00, // Purple
    0x00000f00, // Teal
};

uint32_t green_color[STR_LEN] = {
    0x0f000000, // Green
    0x0f000000, // Red
    0x0f000000, // Blue
    0x0f000000, // White
    0x0f000000, // White (via RGB)
    0x0f000000, // Yellow
    0x0f000000, // Purple
    0x0f000000, // Teal

};

uint32_t red_color[STR_LEN] = {
    0x000f0000, // Green
    0x000f0000, // Red
    0x000f0000, // Blue
    0x000f0000, // White
    0x000f0000, // White (via RGB)
    0x000f0000, // Yellow
    0x000f0000, // Purple
    0x000f0000, // Teal

};

static volatile void *getPruMmapAddr(void)
{
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1)
    {
        perror("ERROR: could not open /dev/mem");
        exit(EXIT_FAILURE);
    }
    // Points to start of PRU memory.
    volatile void *pPruBase = mmap(0, PRU_LEN, PROT_READ | PROT_WRITE,
                                   MAP_SHARED, fd, PRU_ADDR);
    if (pPruBase == MAP_FAILED)
    {
        perror("ERROR: could not map memory");
        exit(EXIT_FAILURE);
    }
    close(fd);
    return pPruBase;
}
static void freePruMmapAddr(volatile void *pPruBase)
{
    if (munmap((void *)pPruBase, PRU_LEN))
    {
        perror("PRU munmap failed");
        exit(EXIT_FAILURE);
    }
}

uint32_t* create_led_color(double y_position) {
    uint32_t* color_array = (uint32_t*)malloc(STR_LEN * sizeof(uint32_t));
    if (color_array == NULL) {
        // Handle memory allocation error
        return NULL;
    }

    // Set the LEDs based on the y_position
    if (y_position < -0.03) {
        // Bottom LEDs blue
        for (int i = 0; i < STR_LEN; ++i) {
            color_array[i] = (i < 3) ? 0x00000f00 : 0x00000000;
        }
    } else if (y_position > 0.03) {
        // Top LEDs blue
        for (int i = 0; i < STR_LEN; ++i) {
            color_array[i] = (i >= STR_LEN - 3) ? 0x00000f00 : 0x00000000;
        }
    } else {
        // Middle LEDs blue
        int start_index = (STR_LEN - 3) / 2;
        for (int i = 0; i < STR_LEN; ++i) {
            color_array[i] = (i >= start_index && i < start_index + 3) ? 0x00000f00 : 0x00000000;
        }
    }

    return color_array;
}


void set_led_color(uint32_t *led_color, double position_diff) {
    // Turn off all LEDs

uint32_t color[8] = {0};

printf("%f\n", position_diff);

    // Calculate the index of the first LED in the target range
    int target_start_index;
    if (position_diff < -0.03) {
        target_start_index = 0; // Bottom LEDs
    } else if (position_diff > 0.03) {
        target_start_index = 8 - 3; // Top LEDs
    } else {
        target_start_index = (8 - 3) / 2; // Middle LEDs
    }

    //Turn on the target LEDs
    for (int i = target_start_index; i < target_start_index + 3; ++i) {
        color[i] = 0x000f0000; // Set to red color
    }
    
        memcpy(led_color, color, sizeof(color));

}

void *neopixel(void *args)
{
    (void)args;
    volatile void *pPruBase = getPruMmapAddr();
    volatile sharedMemStruct_t *pSharedPru0 = PRU0_MEM_FROM_BASE(pPruBase);

    while (!shutdown)
    {
        // if (getXpositiondiff() < -0.03)
        // {
        //     memcpy(pSharedPru0->ledColor, green_color, sizeof(green_color));
        // }
        // else if (getXpositiondiff() > 0.03)
        // {
        //     memcpy(pSharedPru0->ledColor, red_color, sizeof(red_color));
        // }
        // else
        // {
        //     memcpy(pSharedPru0->ledColor, blue_color, sizeof(blue_color));
        // }

                    //memcpy(pSharedPru0->ledColor, create_led_color(getYpositiondiff()), sizeof(blue_color));

                set_led_color(pSharedPru0->ledColor, getYpositiondiff());

        sleepForMs(10);
    }

    // Turn off all LEDs
    for (int i = 0; i < STR_LEN; i++)
    {
        pSharedPru0->ledColor[i] = 0x00000000;
    }
    freePruMmapAddr(pPruBase);
    return NULL;
}

void neopixelInit()
{
    runCommand("config-pin P8.11 pruout");
    pthread_create(&neopixelThread, NULL, neopixel, NULL);
}

void neopixelShutdown()
{
    shutdown = true;
    pthread_join(neopixelThread, NULL);
}