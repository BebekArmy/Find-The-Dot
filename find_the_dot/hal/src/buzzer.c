#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include "hal/general_command.h"

#define PWM_LED_PATH "/dev/bone/pwm/0/a"

static bool shutdown = false;
static pthread_t buzzerThread;
int sound = 0;

static void initializeBuzzer(){
    runCommand("config-pin P9_22 pwm");
    writeToFile(PWM_LED_PATH "/enable", "0");
}

void playFrequency(int frequency, int durationMs) {
    int period = 1000000000 / frequency;
    int dutyCycle = period / 2; 

    char periodStr[20];
    char dutyStr[20];

    sprintf(periodStr, "%d", period);
    sprintf(dutyStr, "%d", dutyCycle);

    writeToFile(PWM_LED_PATH "/period", periodStr);
    writeToFile(PWM_LED_PATH "/duty_cycle", dutyStr);
    writeToFile(PWM_LED_PATH "/enable", "1");

    sleepForMs(durationMs);

    writeToFile(PWM_LED_PATH "/enable", "0");
    writeToFile(PWM_LED_PATH "/duty_cycle", "0");
}

void playHitSound() {
    int hitFrequencies[] = {440, 660, 440, 600};
    for (int i = 0; i < sizeof(hitFrequencies)/sizeof(hitFrequencies[0]); i++) {
        playFrequency(hitFrequencies[i], 125); 
    }
}

void playMissSound() {
    int missFrequencies[] = {120, 60, 120, 60};
    for (int i = 0; i < sizeof(missFrequencies)/sizeof(missFrequencies[0]); i++) {
        playFrequency(missFrequencies[i], 125);
    }
}


void *setBuzzer(void *args){
    (void)args;
    
    while(!shutdown){
        if(sound == 1){
            playHitSound();
            sound = 0;
        } 
        else if(sound == 2){
            playMissSound();
            sound = 0;
        }

    }
    writeToFile(PWM_LED_PATH "/enable", "0");
    return NULL;
}

void buzzerInit(){
    initializeBuzzer();
    pthread_create(&buzzerThread, NULL, setBuzzer, NULL);
}

void buzzerShutdown(){
    shutdown = true;
    pthread_join(buzzerThread, NULL);
}

void playHit(){
    sound = 1;
}

void playMiss(){
    sound = 2;
}
