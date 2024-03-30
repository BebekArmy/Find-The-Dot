#include "hal/display.h"
#include "hal/general_command.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>


#define I2CDRV_LINUX_BUS0 "/dev/i2c-0" 
#define I2CDRV_LINUX_BUS1 "/dev/i2c-1" 
#define I2CDRV_LINUX_BUS2 "/dev/i2c-2" 

#define GPIO61_value "/sys/class/gpio/gpio61/value"
#define GPIO44_value "/sys/class/gpio/gpio44/value"

#define GPIO61_direction "/sys/class/gpio/gpio61/direction"
#define GPIO44_direction "/sys/class/gpio/gpio44/direction"

#define GPIO61_export "/sys/class/gpio/export"
#define GPIO44_export "/sys/class/gpio/export"

#define I2C_DEVICE_ADDRESS 0x20 
#define REG_DIRA 0x00 // Zen Red uses: 0x02 
#define REG_DIRB 0x01 // Zen Red uses: 0x03 

static char DIRA[10] = {0xd1, 0xc0, 0x98, 0xd8, 0xc8, 0x58, 0x58, 0x01, 0xd8, 0xd8};
static char DIRB[10] = {0xa5, 0x00, 0x83, 0x03, 0x22, 0x23, 0xa3, 0x05, 0xa3, 0x23};

static int digit_left = 0;
static int digit_right = 0;

static pthread_t displayThread;

static bool shutdown = false;

static int initI2cBus(char* bus, int address) { 
    int i2cFileDesc = open(bus, O_RDWR); 
    int result = ioctl(i2cFileDesc, I2C_SLAVE, address); 
    if (result < 0) { 
        perror("I2C: Unable to set I2C device to slave address."); 
        exit(1); 
    } 
    return i2cFileDesc; 
}

static void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value) { 
    unsigned char buff[2]; 
    buff[0] = regAddr; 
    buff[1] = value; 
    int res = write(i2cFileDesc, buff, 2); 
    if (res != 2) { 
        perror("I2C: Unable to write i2c register."); 
        exit(1); 
    } 
}

void initializeDisplay(){
    runCommand("config-pin P9_18 i2c");
    runCommand("config-pin P9_17 i2c");

    //writeToFile(GPIO61_export, "61");
    //writeToFile(GPIO44_export, "44");

    writeToFile(GPIO61_direction, "out");
    writeToFile(GPIO44_direction, "out");

    writeToFile(GPIO61_value, "1");
    writeToFile(GPIO44_value, "1");

    int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);

    writeI2cReg(i2cFileDesc, 0x02 , 0x00);
    writeI2cReg(i2cFileDesc, 0x03 , 0x00);

    writeI2cReg(i2cFileDesc, REG_DIRA, DIRA[0]); 
    writeI2cReg(i2cFileDesc, REG_DIRB, DIRB[0]);


}

void setDisplay(int value) {
    // Check if value is greater than 99
    if (value > 99) {
        digit_left = 9;
        digit_right = 9;
    }
    // Check if value is less than 10
    else if (value < 10) {
        digit_left = 0;
        digit_right = value;
    }
    // Otherwise, set digits accordingly
    else {
        digit_left = value / 10;
        digit_right = value % 10;
    }
}

void *displayDigits(void *args) {
    (void)args;
    int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS); 
    while (!shutdown) {
        
        writeToFile(GPIO61_value, "0");
        writeToFile(GPIO44_value, "0");

        writeI2cReg(i2cFileDesc, REG_DIRA, DIRA[digit_left]); 
        writeI2cReg(i2cFileDesc, REG_DIRB, DIRB[digit_left]);
        writeToFile(GPIO61_value, "1"); 
        sleepForMs(5);

        writeToFile(GPIO61_value, "0");
        writeToFile(GPIO44_value, "0");

        writeI2cReg(i2cFileDesc, REG_DIRA, DIRA[digit_right]); 
        writeI2cReg(i2cFileDesc, REG_DIRB, DIRB[digit_right]);

        writeToFile(GPIO44_value, "1"); 
        sleepForMs(5);
    }
    writeToFile(GPIO61_value, "0");
    writeToFile(GPIO44_value, "0");
    close(i2cFileDesc);
    
    return NULL;
}

void createDisplayThread() {
    //printf("Creating Display Thread\n");
    pthread_create(&displayThread, NULL, displayDigits, NULL);
}

void joinDisplayThread() {
    //printf("Joining Display Thread\n");
    pthread_join(displayThread, NULL);
}

void shutdownDisplay() {
    //printf("Shutting down Display\n");
    shutdown = true;
}