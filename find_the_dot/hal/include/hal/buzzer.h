#ifndef _BUZZER_H_
#define _BUZZER_H_

/* Module to initialize and cleanup the PWMLED thread.
Supports controlling the LED with the potentiometer. Repeatedly writes the period and duty_cycle parameters of the LED
based on the voltage read from the potentiometer. 
*/

#include <stdint.h>

void buzzerInit();
void buzzerShutdown();
void playHit();
void playMiss();

#endif