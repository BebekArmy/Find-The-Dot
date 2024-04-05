#include "hal/accelerometer.h"
#include "hal/general_command.h"
#include "../../app/include/joystick_pru.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <math.h>

#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"

#define I2C_DEVICE_ADDRESS 0x18

#define REG_X_L 0x28
#define REG_X_H 0x29
#define REG_Y_L 0x2A
#define REG_Y_H 0x2B
#define REG_Z_L 0x2C
#define REG_Z_H 0x2D

#define ACCELEROMETER_CONVERSION 6555
#define MIN_VALUE -0.5
#define MAX_VALUE 0.5

static pthread_t accelerometerThread;

static bool shutdown = false;
static double x_target = 0.0;
static double y_target = 0.0;

static double x_position_difference = 0.0;
static double y_position_difference = 0.0;

double getXtarget()
{
    return x_target;
}

double getYtarget()
{
    return y_target;
}

double getXpositiondiff()
{
    return x_position_difference;
}

double getYpositiondiff()
{
    return y_position_difference;
}

static int initI2cBus(char *bus, int address)
{
    int i2cFileDesc = open(bus, O_RDWR);
    int result = ioctl(i2cFileDesc, I2C_SLAVE, address);
    if (result < 0)
    {
        perror("I2C: Unable to set I2C device to slave address.");
        exit(1);
    }
    return i2cFileDesc;
}

static void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value)
{
    unsigned char buff[2];
    buff[0] = regAddr;
    buff[1] = value;
    int res = write(i2cFileDesc, buff, 2);
    if (res != 2)
    {
        perror("I2C: Unable to write i2c register.");
        exit(1);
    }
}

static unsigned char readI2cReg(int i2cFileDesc, unsigned char regAddr)
{
    // To read a register, must first write the address
    int res = write(i2cFileDesc, &regAddr, sizeof(regAddr));
    if (res != sizeof(regAddr))
    {
        perror("I2C: Unable to write to i2c register.");
        exit(1);
    }
    // Now read the value and return it
    char value = 0;
    res = read(i2cFileDesc, &value, sizeof(value));
    if (res != sizeof(value))
    {
        perror("I2C: Unable to read from i2c register");
        exit(1);
    }
    return value;
}

void initializeAccelerometer()
{
    runCommand("config-pin P9_18 i2c");
    runCommand("config-pin P9_17 i2c");

    int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);

    writeI2cReg(i2cFileDesc, 0x20, 0x27);
}

double readAccelerometerAxis(int i2cFileDesc, unsigned char regLSB, unsigned char regMSB)
{
    // Accelerometer readings
    unsigned char buff[2];

    // Read low byte
    buff[0] = readI2cReg(i2cFileDesc, regLSB);
    // Read high byte
    buff[1] = readI2cReg(i2cFileDesc, regMSB);

    // Combine low and high bytes to get full 16-bit value
    int16_t axisValue = (buff[1] << 8) | buff[0];

    // scale to range [-0.5, 0.5]
    return ((double)axisValue / 16384) * 0.5;
}

void setRandomTarget()
{
    // printf("X: %f, Y: %f\n", x, y);
    x_target = random_double(MIN_VALUE, MAX_VALUE);
    y_target = random_double(MIN_VALUE, MAX_VALUE);
}

void setPositionDifference(double x, double y)
{
    // printf("X: %f, Y: %f\n", x, y);
    x_position_difference = x;
    y_position_difference = y;
}

void *updateAccelerometerReading(void *args)
{
    (void)args;
    int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);
 

    // setRandomTarget();

    while (!shutdown)
    {
        if (isJoystickDownPressed() && x_position_difference == 0 && y_position_difference == 0)
        {
          
            // setRandomTarget();
        }
        double x = readAccelerometerAxis(i2cFileDesc, REG_X_L, REG_X_H);
        double y = readAccelerometerAxis(i2cFileDesc, REG_Y_L, REG_Y_H);


        setPositionDifference((double)x - x_target, (double)y - y_target);

        // setPositionDifference(fabs((double)x-x_target), fabs((double)y-y_target));
        // printf("X: %f, Y: %f\n", x_position_difference, y_position_difference);

        sleepForMs(5);
    }

    close(i2cFileDesc);

    return NULL;
}

void createAccelerometerThread()
{
    pthread_create(&accelerometerThread, NULL, updateAccelerometerReading, NULL);
}

void joinAccelerometerThread()
{
    pthread_join(accelerometerThread, NULL);
}

void shutdownAccelerometer()
{
    shutdown = true;
}