#ifndef _JOYSTICK_PRU_H_
#define _JOYSTICK_PRU_H_

/* Module to initialize and cleanup the joystick thread.

*/

#include <stdint.h>
#include <stdbool.h>

bool isJoystickRightPressed();
bool isJoystickDownPressed();
void joystickInit();
void joystickShutdown();

#endif