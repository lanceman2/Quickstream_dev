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
        "      --maxWrite BYTES      default value %zu\n"
        "\n"
        "\n",
        QS_DEFAULTMAXWRITE);
}


static size_t maxWrite;


int construct(int argc, const char **argv) {

    DSPEW();

    maxWrite = qsOptsGetSizeT(argc, argv,
            "maxWrite", QS_DEFAULTMAXWRITE);
  
    return 0; // success
}


int start(uint32_t numInPorts, uint32_t numOutPorts) {

    ASSERT(numInPorts == 1, "");
    ASSERT(numOutPorts, "");
    DSPEW("count=%d   %" PRIu32 " inputs and  %" PRIu32 " outputs",
            count++, numInPorts, numOutPorts);

    return 0; // success
}


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInPorts, uint32_t numOutPorts) {

    DASSERT(lens, "");
    DASSERT(numInPorts == 1, "");
    DASSERT(numOutPorts, "");
    DASSERT(lens[0], "");

    size_t len = lens[0];
    if(len > maxWrite)
        len = maxWrite;

    for(uint32_t i=0; i<numOutPorts; ++i) {
        memcpy(qsGetOutputBuffer(i, len, len), buffers[0], len);
        qsOutput(i, len);
    }

    qsAdvanceInput(0, len);
    
    return 0; // success
}
