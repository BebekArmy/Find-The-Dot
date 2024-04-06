#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include "shutdown.h"
#include "joystick_pru.h"


int main(void) {

    createThreads();
    waitShutdown();
    joinThreads();

    return 0;
}
