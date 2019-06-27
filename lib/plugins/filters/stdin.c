#include <stdio.h>

#include "../../../include/fsf.h"
#include "../../../lib/debug.h"


int constructor(void) {

    DSPEW();

    return 0; // success
}

int destructor(void) {

    return 0; // success
}

int input(void *buffer, size_t len, int inputChannelNum) {

    return 0; // success
}

int start(int numInChannels, int numOutChannels) {

    return 0; // success
}

int stop(int numInChannels, int numOutChannels) {

    return 0; // success
}

