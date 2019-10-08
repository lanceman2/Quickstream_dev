// This tests all the callbacks: help(), construct(), destroy(), start(),
// stop() and input().  Only help() and input() are required.
#include <unistd.h>
#include <string.h>

#include "../../../../../include/qsfilter.h"
#include "../../../../../lib/debug.h"

#ifdef SPEW_LEVEL_DEBUG
static int count = 0;
#endif


// TODO: Add command line options.

static unsigned int usecs = 1000000; // 1 micro second = 1s/1,000,000


void help(FILE *f) {
    fprintf(f, "the test filter module that sleeps\n");
}

int construct(int argc, const char **argv) {

    DSPEW("count=%d", count++);
    return 0; // success
}

int destroy(void) {

    DSPEW("count=%d", count++);
    return 0; // success
}

int input(void *buffer, size_t len, uint32_t inputChannelNum,
        uint32_t flowState) {

    // The filter module's stupid action, sleep.
    usleep(usecs);

    // For output buffering.  By this module using default buffering this
    // will be all the output buffers for all output channels.
    void *oBuffer = qsGetBuffer(0);

    // Input and output must be the same length so we use the lesser of
    // the two lengths.
    if(QS_DEFAULTWRITELENGTH < len)
        len = QS_DEFAULTWRITELENGTH;

    memcpy(oBuffer, buffer, len);
    qsAdvanceInput(len);
    qsOutput(len, 0);

    DSPEW("count=%d", count++);
    return 0; // success
}

int start(uint32_t numInChannels, uint32_t numOutChannels) {

    DSPEW("count=%d   %" PRIu32 " inputs and  %" PRIu32 " outputs",
            count++, numInChannels, numOutChannels);

    if(!numInChannels || !numOutChannels) {
        ERROR("There must be at least 1 input and 1 output.\n");
        return 1;
    }

    return 0; // success
}

int stop(uint32_t numInChannels, uint32_t numOutChannels) {

    DSPEW("count=%d", count++);
    return 0; // success
}

