#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define QS_FILTER_NAME_CODE
#include "../../../../../include/qsfilter.h"
#include "../../../../../lib/debug.h"

#ifdef SPEW_LEVEL_DEBUG
static int count = 0;
#endif



void help(FILE *f) {
    fprintf(f,
        "test filter module that copies all input to each output\n"
        "\n"
        "                               OPTIONS\n"
        "\n"
        "      --maxReadThreshold BYTES  default value %zu\n"
        "\n"
        "\n"
        "      --minReadThreshold BYTES  default value %zu\n"
        "\n"
        "\n"
        "      --maxReadSize BYTES       default value %zu\n"
        "\n"
        "\n",
        QS_DEFAULT_maxReadThreshold,
        QS_DEFAULT_minReadThreshold,
        QS_DEFAULT_maxReadSize);
}


static size_t maxReadThreshold, minReadThreshold, maxReadSize;


int construct(int argc, const char **argv) {

    DSPEW();

    maxReadThreshold = qsOptsGetSizeT(argc, argv,
            "maxReadThreshold", QS_DEFAULT_maxReadThreshold);
    minReadThreshold = qsOptsGetSizeT(argc, argv,
            "minReadThreshold", QS_DEFAULT_minReadThreshold);
    maxReadSize = qsOptsGetSizeT(argc, argv,
            "maxReadSize", QS_DEFAULT_maxReadSize);
 
    return 0; // success
}


int start(uint32_t numInChannels, uint32_t numOutChannels) {

    DSPEW("BASE_FILE=%s", __BASE_FILE__);

    DSPEW("count=%d   %" PRIu32 " inputs and  %" PRIu32 " outputs",
            count++, numInChannels, numOutChannels);

    // We needed a start() to check for this error.
    if(!numInChannels || !numOutChannels) {
        ERROR("There must be at least 1 input and 1 output.\n");
        return 1;
    }

    qsSetMaxReadThreshold(maxReadThreshold, QS_ALLCHANNELS);
    qsSetMinReadThreshold(minReadThreshold, QS_ALLCHANNELS);
    qsSetMaxReadSize(maxReadSize, QS_ALLCHANNELS);

    return 0; // success
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
