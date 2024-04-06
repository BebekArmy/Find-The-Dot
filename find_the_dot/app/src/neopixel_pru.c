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
#include "hal/buzzer.h"
#include "hal/display.h"
#include "../pru-as4/Linux/sharedDataStruct.h"
#include "shutdown.h"
#include "joystick_pru.h"

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

#define GREEN 0x0f000000
#define BLUE 0x00000f00
#define RED 0x000f0000
#define BRIGHT_GREEN 0xff000000
#define BRIGHT_BLUE 0x0000ff00
#define BRIGHT_RED 0x00ff0000

static bool shutdown = false;
static int score = 0;
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

uint32_t blue_color[STR_LEN] = {
    BRIGHT_BLUE,
    BRIGHT_BLUE, 
    BRIGHT_BLUE, 
    BRIGHT_BLUE, 
    BRIGHT_BLUE, 
    BRIGHT_BLUE,
    BRIGHT_BLUE, 
    BRIGHT_BLUE, 
};

uint32_t green_color[STR_LEN] = {
    BRIGHT_GREEN,
    BRIGHT_GREEN, 
    BRIGHT_GREEN, 
    BRIGHT_GREEN, 
    BRIGHT_GREEN, 
    BRIGHT_GREEN,
    BRIGHT_GREEN, 
    BRIGHT_GREEN, 
};

uint32_t red_color[STR_LEN] = {
    BRIGHT_RED,
    BRIGHT_RED, 
    BRIGHT_RED, 
    BRIGHT_RED, 
    BRIGHT_RED, 
    BRIGHT_RED,
    BRIGHT_RED, 
    BRIGHT_RED, 
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

void setColor(uint32_t *color, int current_color, int start_index)
{
    color[start_index] = current_color;

    if (current_color == GREEN)
    {
        color[start_index + 1] = BRIGHT_GREEN;
    }
    else if (current_color == RED)
    {
        color[start_index + 1] = BRIGHT_RED;
    }
    else if (current_color == BLUE)
    {
        color[start_index + 1] = BRIGHT_BLUE;
    }
    color[start_index + 2] = current_color;
}

void set_led_color(uint32_t *led_color, double x_position_diff, double y_position_diff)
{
    uint32_t color[8] = {0};
    int current_color = 0x00000000;

    // range [-0.5, 0.5] divided into 11 possible zones
    double zone_increment = 1 / 11.0;


    if (isJoystickDownPressed()) {
            playMiss();
    }

    if (y_position_diff < -0.07)
    {
        current_color = GREEN;
    }
    else if (y_position_diff > 0.07)
    {
        current_color = RED;
    }
    else
    {
        current_color = BLUE;
    }


    if (x_position_diff <= -0.5 + zone_increment)
    {
        color[0] = current_color;
    }
    else if (x_position_diff <= -0.5 + 2 * zone_increment && x_position_diff > -0.5 + zone_increment)
    {
        if (current_color == GREEN)
        {
            color[1] = BRIGHT_GREEN;
        }
        else if (current_color == RED)
        {
            color[1] = BRIGHT_RED;
        }
        else if (current_color == BLUE)
        {
            color[1] = BRIGHT_BLUE;
        }
        color[0] = current_color;
    }
    else if (x_position_diff <= -0.5 + 3 * zone_increment && x_position_diff > -0.5 + 2 * zone_increment)
    {
        setColor(&color, current_color, 0);
    }
    else if (x_position_diff <= -0.5 + 4 * zone_increment && x_position_diff > -0.5 + 3 * zone_increment)
    {
        setColor(&color, current_color, 1);
    }
    else if (x_position_diff <= -0.5 + 5 * zone_increment && x_position_diff > -0.5 + 4 * zone_increment)
    {
        setColor(&color, current_color, 2);
    }
    // On Target vertically
    else if (x_position_diff <= -0.5 + 6 * zone_increment && x_position_diff > -0.5 + 5 * zone_increment)
    {

        if (current_color == GREEN)
        {
            memcpy(led_color, green_color, sizeof(color));
            return;
        }
        else if (current_color == RED)
        {
            memcpy(led_color, red_color, sizeof(color));
            return;
        }
        else if (current_color == BLUE)
        {

            memcpy(led_color, blue_color, sizeof(color));

            if (isJoystickDownPressed()) {
                playHit();
                setRandomTarget();
                setDisplay(++score);
                sleepForMs(500);
            }

            return;
        }
    }
    else if (x_position_diff <= -0.5 + 7 * zone_increment && x_position_diff > -0.5 + 6 * zone_increment)
    {
        setColor(&color, current_color, 3);
    }
    else if (x_position_diff <= -0.5 + 8 * zone_increment && x_position_diff > -0.5 + 7 * zone_increment)
    {
        setColor(&color, current_color, 4);
    }
    else if (x_position_diff <= -0.5 + 9 * zone_increment && x_position_diff > -0.5 + 8 * zone_increment)
    {
        setColor(&color, current_color, 5);
    }
    else if (x_position_diff <= -0.5 + 10 * zone_increment && x_position_diff > -0.5 + 9 * zone_increment)
    {
        if (current_color == GREEN)
        {
            color[6] = BRIGHT_GREEN;
        }
        else if (current_color == RED)
        {
            color[6] = BRIGHT_RED;
        }
        else if (current_color == BLUE)
        {
            color[6] = BRIGHT_BLUE;
        }

        color[7] = current_color;
    }
    else if (x_position_diff > -0.5 + 10 * zone_increment)
    {
        color[7] = current_color;
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

        set_led_color(pSharedPru0->ledColor, getYpositiondiff(), getXpositiondiff());
        if(isJoystickRightPressed()){
            signalShutdown();
        }
        sleepForMs(90);
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
