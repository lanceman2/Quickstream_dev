#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define QS_FILTER_NAME_CODE
#include "../../../../../include/quickstream/filter.h"
#include "../../../../../lib/debug.h"



void help(FILE *f) {
    fprintf(f,
        "test filter module that copies 1 input to each output\n"
        "There may be no outputs.  There must be 1 input.\n"
        "\n"
        "                       OPTIONS\n"
        "\n"
        "      --maxRead BYTES      default value %zu\n"
        "\n"
        "\n",
        QS_DEFAULTMAXWRITE);
}


static size_t maxRead;
static uint64_t count;


int construct(int argc, const char **argv) {

    DSPEW();

    maxRead = qsOptsGetSizeT(argc, argv,
            "maxRead", QS_DEFAULTMAXWRITE);

    maxRead += maxRead%8;
  
    return 0; // success
}


int start(uint32_t numInPorts, uint32_t numOutPorts) {

    ASSERT(numInPorts == 1, "");
    DSPEW("%" PRIu32 " inputs and  %" PRIu32 " outputs",
            numInPorts, numOutPorts);

    qsSetInputReadPromise(0, maxRead);

    for(uint32_t i=0; i<numOutPorts; ++i)
        qsCreateOutputBuffer(i, maxRead);

    count = 0;

    return 0; // success
}


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInPorts, uint32_t numOutPorts) {

    DASSERT(lens, "");
    DASSERT(numInPorts == 1, "");
    DASSERT(lens[0], "");

//DSPEW(" ++++++++++++++++++++++++++++ inputLen=%zu", lens[0]);

    size_t len = lens[0];
    if(len > maxRead)
        len = maxRead;

    len -= len%8;

    if(len == 0) return 0;

    size_t num = len/8;

    uint64_t *in = buffers[0];

    for(size_t i=0; i<num; ++i) {
        ASSERT(count == in[i], "(%zu/%zu) count %" PRIu64 " != %"
                PRIu64 " failed ", i, num, count, in[i]);
        ++count;
    }

    qsAdvanceInput(0, len);

    for(uint32_t i=0; i<numOutPorts; ++i) {
        memcpy(qsGetOutputBuffer(i, len, len), in, len);
        qsOutput(i, len);
    }

    return 0; // success
}
