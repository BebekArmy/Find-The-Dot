#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include "shutdown.h"


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

int main(void) {

    createThreads();
    // waitShutdown();
    //testBuzzer();

    joinThreads();

    return 0;
}
