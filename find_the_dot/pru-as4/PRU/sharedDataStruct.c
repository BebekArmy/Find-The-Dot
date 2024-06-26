/*
    NeoPixel RGBW demo program for 8 LED unit such as:
        https://www.adafruit.com/product/2868
    NOTE: This is RGBW, not RGB!

    Install process
    - Put the NeoPixel into a breadboard
    - Connect the NeoPixel with 3 wires:
        - Connect NeoPixel "GND" and "DIN" (data in) to the 3-pin "LEDS" header on Zen
            Zen Cape's LEDS header:
                Pin 1: DIN (Data): left most pin; beside USB-micro connection, connects to P8.11
                Pin 2: GND (Ground): middle pin
                Pin 3: Unused (it's "5V external power", which is not powered normally on the BBG)
        - Connect NeoPixel "5VDC" to P9.7 or P9.8
            Suggest using the header-extender to make it easier to make a good connection.
        - OK to wire directly to BBG: no level shifter required.
    - Software Setup
        - On Host
            make      # on parent folder to copy to NFS
        - On Target: 
            config-pin P8.11 pruout
            make
            make install_PRU0
    - All lights should light up on the LED strip

    Based on code from the PRU Cookbook by Mark A. Yoder:
        https://beagleboard.org/static/prucookbook/#_setting_neopixels_to_different_colors
*/

#include <stdint.h>
#include <pru_cfg.h>
#include <string.h>
#include "../Linux/sharedDataStruct.h"
#include "resource_table_empty.h"

#define STR_LEN         8       // # LEDs in our string
#define oneCyclesOn     700/5   // Stay on 700ns
#define oneCyclesOff    800/5
#define zeroCyclesOn    350/5
#define zeroCyclesOff   600/5
#define resetCycles     60000/5 // Must be at least 50u, use 60u

// P8_11 for output (on R30), PRU0
#define DATA_PIN 15       // Bit number to output on

volatile register uint32_t __R30;
volatile register uint32_t __R31;

// GPIO Input: P8_15 = pru0_pru_r31_15 
//   = JSRT (Joystick Right) on Zen Cape
//   (Happens to be bit 15 and p8_15;
#define JOYSTICK_RIGHT_MASK (1 << 15)

// GPIO Input: P8_16 = pru0_pru_r31_14
//   = JSDW (Joystick Down) on Zen Cape
//   (Happens to be bit 14 and p8_16;
#define JOYSTICK_DOWN_MASK (1 << 14)

#define THIS_PRU_DRAM       0x00000         // Address of DRAM
#define OFFSET              0x200           // Skip 0x100 for Stack, 0x100 for Heap (from makefile)
#define THIS_PRU_DRAM_USABLE (THIS_PRU_DRAM + OFFSET)

// This works for both PRU0 and PRU1 as both map their own memory to 0x0000000
volatile sharedMemStruct_t *pSharedMemStruct = (volatile void *)THIS_PRU_DRAM_USABLE;

void main(void)
{
    // Clear SYSCFG[STANDBY_INIT] to enable OCP master port
    CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

    // COLOURS
    // - 1st element in array is 1st (bottom) on LED strip; last element is last on strip (top)
    // - Bits: {Green/8 bits} {Red/8 bits} {Blue/8 bits} {White/8 bits}
    uint32_t init_color[STR_LEN] = {
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000
    };

    memcpy(pSharedMemStruct->ledColor, init_color, sizeof(init_color));

    uint32_t color[STR_LEN];


    __delay_cycles(resetCycles);

    while(true) {
        pSharedMemStruct->isJoyStickRightPressed = (__R31 & JOYSTICK_RIGHT_MASK) != 0;
        pSharedMemStruct->isjoyStickDownPressed = (__R31 & JOYSTICK_DOWN_MASK) != 0;
        memcpy(color, pSharedMemStruct->ledColor, sizeof(color));
        for(int j = 0; j < STR_LEN; j++) {
            for(int i = 31; i >= 0; i--) {
                
                if(color[j] & ((uint32_t)0x1 << i)) {
                    __R30 |= 0x1<<DATA_PIN;      // Set the GPIO pin to 1
                    __delay_cycles(oneCyclesOn-1);
                    __R30 &= ~(0x1<<DATA_PIN);   // Clear the GPIO pin
                    __delay_cycles(oneCyclesOff-2);
                } else {
                    __R30 |= 0x1<<DATA_PIN;      // Set the GPIO pin to 1
                    __delay_cycles(zeroCyclesOn-1);
                    __R30 &= ~(0x1<<DATA_PIN);   // Clear the GPIO pin
                    __delay_cycles(zeroCyclesOff-2);
                }
            }
        }

        // Send Reset
        __R30 &= ~(0x1<<DATA_PIN);   // Clear the GPIO pin
        __delay_cycles(resetCycles);

    }

    // // Turn off all LEDs
    // for(int i = 0; i < STR_LEN; i++) {
    //     for(int j = 31; j >= 0; j--) {
    //         __R30 |= 0x1<<DATA_PIN;      // Set the GPIO pin to 1
    //         __delay_cycles(zeroCyclesOn-1);
    //         __R30 &= ~(0x1<<DATA_PIN);   // Clear the GPIO pin
    //         __delay_cycles(zeroCyclesOff-2);
    //     }
    // }

    // __R30 &= ~(0x1<<DATA_PIN);   // Clear the GPIO pin
    // __delay_cycles(resetCycles);

    // __halt();
}