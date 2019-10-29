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
        "      --maxRead BYTES       default value %zu\n"
        "\n"
        "      --maxWrite BYTES      default value %zu\n"
        "\n"
        "\n",
        QS_DEFAULT_MAXREADTHRESHOLD,
        QS_DEFAULT_MINREADTHRESHOLD,
        QS_DEFAULT_MAXREAD,
        QS_DEFAULT_MAXWRITE);
}


static size_t maxReadThreshold, minReadThreshold, maxRead, maxWrite;


int construct(int argc, const char **argv) {

    DSPEW();

    maxReadThreshold = qsOptsGetSizeT(argc, argv,
            "maxReadThreshold", QS_DEFAULT_MAXREADTHRESHOLD);
    minReadThreshold = qsOptsGetSizeT(argc, argv,
            "minReadThreshold", QS_DEFAULT_MINREADTHRESHOLD);
    maxRead = qsOptsGetSizeT(argc, argv,
            "maxRead", QS_DEFAULT_MAXREAD);
    maxWrite = qsOptsGetSizeT(argc, argv,
            "maxWrite", QS_DEFAULT_MAXWRITE);
  
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
    qsSetMaxRead(maxRead, QS_ALLCHANNELS);
    qsBufferCreate(maxWrite, QS_ALLCHANNELS);

    return 0; // success
}


int input(void *buffer, size_t len, uint32_t inputChannelNum,
        uint32_t flowState) {

    DASSERT(len, "");


    //DSPEW(" --------------maxRead=%zu-------- LEN=%zu", maxRead, len);

    // For output buffering.  By this module using default buffering this
    // will be all the output buffers for all output channels.
    void *oBuffer = qsGetBuffer(0);

    // Input and output must be the same length so we use the lesser of
    // the two lengths.
    if(maxWrite < len) {
        len = maxWrite;
        qsAdvanceInput(len);
    }

    // TODO: change this to a pass-through.

    memcpy(oBuffer, buffer, len);

    qsOutput(len, 0);

    return 0; // success
}
