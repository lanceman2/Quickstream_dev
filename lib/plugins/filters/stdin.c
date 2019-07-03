#include <stdio.h>

#include "../../../include/fsfilter.h"
#include "../../../lib/debug.h"


int construct(void) {

    DSPEW();

    return 0; // success
}

int destroy(void) {

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

