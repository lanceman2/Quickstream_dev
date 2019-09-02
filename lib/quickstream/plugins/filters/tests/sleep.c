#include <stdio.h>

#include "../../../../../include/qsfilter.h"
#include "../../../../../lib/debug.h"

#ifdef SPEW_LEVEL_DEBUG
static int count = 0;
#endif


void help(FILE *f) {
}

int construct(int argc, const char **argv) {

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

    DSPEW("count=%d   %" PRIu32 " inputs and  %" PRIu32 " outputs",
            count++, numInChannels, numOutChannels);
    return 0; // success
}

int stop(uint32_t numInChannels, uint32_t numOutChannels) {

    DSPEW("count=%d", count++);
    return 0; // success
}

