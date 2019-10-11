// This tests all the callbacks: help(), construct(), destroy(), start(),
// stop() and input().  Only help() and input() are required.
#include <unistd.h>
#include <string.h>

#define QS_FILTER_NAME_CODE
#include "../../../../../include/qsfilter.h"
#include "../../../../../lib/debug.h"

#ifdef SPEW_LEVEL_DEBUG
static int count = 0;
#endif



void help(FILE *f) {
    fprintf(f, "test filter module that copies input to output\n");
}


int input(void *buffer, size_t len, uint32_t inputChannelNum,
        uint32_t flowState) {

    DASSERT(len, "");

    // For output buffering.  By this module using default buffering this
    // will be all the output buffers for all output channels.
    void *oBuffer = qsGetBuffer(0);

    // Input and output must be the same length so we use the lesser of
    // the two lengths.
    if(QS_DEFAULTWRITELENGTH < len) {
        len = QS_DEFAULTWRITELENGTH;
        qsAdvanceInput(len);
    }

    // TODO: change this to a pass-through.

    memcpy(oBuffer, buffer, len);

    qsOutput(len, 0);

    return 0; // success
}


int start(uint32_t numInChannels, uint32_t numOutChannels) {

    DSPEW("BASE_FILE=%s", __BASE_FILE__);

    DSPEW("count=%d   %" PRIu32 " inputs and  %" PRIu32 " outputs",
            count++, numInChannels, numOutChannels);

    if(!numInChannels || !numOutChannels) {
        ERROR("There must be at least 1 input and 1 output.\n");
        return 1;
    }

    return 0; // success
}
