#ifndef _DISPLAY_H_
#define _DISPLAY_H_

/* Module to initialize and cleanup the 14-segment display and display thread that handles it.
Supports displaying numbers 0-99.
*/

#include <stdint.h>

void runCommandDisplay(char* command);
void initializeDisplay();
void setDisplay(int value);
void *displayDigits(void *args);
void createDisplayThread();
void joinDisplayThread();
void shutdownDisplay();

#endif