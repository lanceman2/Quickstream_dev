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
        "                       OPTIONS\n"
        "\n"
        "      --maxRead BYTES       default value %zu\n"
        "\n"
        "      --maxWrite BYTES      default value %zu\n"
        "\n"
        "\n",
        QS_DEFAULT_MAXINPUT,
        QS_DEFAULT_MAXWRITE);
}


static size_t maxRead, maxWrite;


int construct(int argc, const char **argv) {

    DSPEW();

    maxRead = qsOptsGetSizeT(argc, argv,
            "maxRead", QS_DEFAULT_MAXINPUT);
    maxWrite = qsOptsGetSizeT(argc, argv,
            "maxWrite", QS_DEFAULT_MAXWRITE);
  
    return 0; // success
}


int start(uint32_t numInChannels, uint32_t numOutChannels) {

    ASSERT(numInChannels == 1, "");
    ASSERT(numOutChannels, "");

    DSPEW("BASE_FILE=%s", __BASE_FILE__);

    DSPEW("count=%d   %" PRIu32 " inputs and  %" PRIu32 " outputs",
            count++, numInChannels, numOutChannels);

    // We needed a start() to check for this error.
    if(numInChannels != 1 || !numOutChannels) {
        ERROR("There must be 1 input and 1 or more outputs.\n");
        return 1;
    }

    size_t lens[numInChannels];
    lens[0] = maxRead;

    qsSetInputMax(lens);
    qsBufferCreate(maxWrite, QS_ALLPORTS);

    return 0; // success
}


int input(const void *buffers[], const size_t lens_in[],
        const bool isFlushing[],
        uint32_t numInPorts, uint32_t numOutPorts) {

    DASSERT(lens_in, "");
    DASSERT(numInPorts == 1, "");
    DASSERT(numOutPorts, "");
    DASSERT(lens_in[0], "");

    size_t lens[1];
    lens[0] = lens_in[0];



    // Input and output must be the same length so we use the lesser of
    // the two lengths.
    if(maxWrite < lens[0]) {
        lens[0] = maxWrite;
        qsAdvanceInputs(lens);
    }

    //DSPEW(" --------------maxRead=%zu-------- LEN=%zu", maxRead, len);

    // For output buffering.  By this module using default buffering this
    // will be all the output buffers for all output channels.
    void *oBuffer = qsGetBuffer(0, lens[0]);

    memcpy(oBuffer, buffers[0], lens[0]);

    qsOutputs(lens);

    return 0; // success
}
