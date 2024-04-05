#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include "shutdown.h"
#include "joystick_pru.h"


void testBuzzer() {
    char input;
    while (true) {
        printf("Enter 'h' to play hit sound, 'm' to play miss sound, 'q' to quit: ");
        scanf(" %c", &input);
        if (input == 'h') {
            playHit();
        } else if (input == 'm') {
            playMiss();
        } else if (input == 'q') {
            break;
        }
    }
}

void testjoyStick() {
    while (true) {
        if (isJoystickDownPressed()) {
            printf("Joystick Down Pressed\n");
            sleep(1);
        } else if (isJoystickRightPressed()) {
            break;
        }
    }
}

int main(void) {

    createThreads();
    sleep(20);
    joinThreads();

    return 0;
}
