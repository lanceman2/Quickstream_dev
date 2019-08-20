#include <stdio.h>

#include "../../../include/qsfilter.h"
#include "../../../lib/debug.h"

static int count = 0;

int construct(void) {

    DSPEW("count=%d", count++);
    return 0; // success
}

int destroy(void) {

    DSPEW("count=%d", count++);
    return 0; // success
}

int input(void *buffer, size_t len, uint32_t inputChannelNum) {

    DSPEW("count=%d", count++);
    return 0; // success
}

int start(uint32_t numInChannels, uint32_t numOutChannels) {

    DSPEW("count=%d", count++);
    return 0; // success
}

int stop(uint32_t numInChannels, uint32_t numOutChannels) {

    DSPEW("count=%d", count++);
    return 0; // success
}

