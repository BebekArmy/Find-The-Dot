#include "hal/accelerometer.h"
#include "hal/display.h"
#include "hal/buzzer.h"

#include "neopixel_pru.h"
#include "joystick_pru.h"

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

pthread_mutex_t shutdownMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t shutdownCond = PTHREAD_COND_INITIALIZER;

void createThreads()
{
    neopixelInit();
    joystickInit();
    buzzerInit();

}

void joinThreads()
{
    neopixelShutdown();
    joystickShutdown();
    buzzerShutdown();
}

void waitShutdown()
{
    pthread_mutex_lock(&shutdownMutex);
    pthread_cond_wait(&shutdownCond, &shutdownMutex);
    pthread_mutex_unlock(&shutdownMutex);
}

void signalShutdown()
{
    pthread_mutex_lock(&shutdownMutex);
    pthread_cond_signal(&shutdownCond);
    pthread_mutex_unlock(&shutdownMutex);
}