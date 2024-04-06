#ifndef _BUZZER_H_
#define _BUZZER_H_

/* Module to initialize and cleanup the Buzzer Thread to play hit and miss sound
*/

#include <stdint.h>

void buzzerInit();
void buzzerShutdown();
void playHit();
void playMiss();

#endif