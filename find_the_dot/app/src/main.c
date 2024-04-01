#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include "shutdown.h"

int main(void) {

    createThreads();
    sleep(5);
    joinThreads();

    return 0;
}
