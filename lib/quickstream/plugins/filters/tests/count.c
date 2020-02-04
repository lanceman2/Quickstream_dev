#include "../../../../../include/quickstream/filter.h"
#include "../../../../../lib/debug.h"


#define DEFAULT_LEN   ((size_t) 8000000)

void help(FILE *f) {

    fprintf(f,
        "This filter is usually a source.\n"
        "This filter will ignore all inputs.\n"
        "This filter write a 64 bit count to all output\n"
        "\n"
        "                  OPTIONS\n"
        "\n"
        "\n"
        "    --maxWrite BYTES   default value %zu.  This value will be\n"
        "                       rounded up to the nearest 8 bytes.\n"
        "                       This is the number of bytes read and written\n"
        "                       for each input() call./n"
        "\n"
         "    --length LEN      Write LEN bytes total and than finish.\n"
         "                      LEN will be rounded up to the nearest\n"
         "                      8 bytes chunck.  The default LEN is %zu.\n"
        "\n", QS_DEFAULTMAXWRITE, DEFAULT_LEN);
}


static size_t maxWrite;
static size_t length, num, total;
static uint64_t count;



int construct(int argc, const char **argv) {

    DSPEW();

    maxWrite = qsOptsGetSizeT(argc, argv,
            "maxWrite", QS_DEFAULTMAXWRITE);
    length = qsOptsGetSizeT(argc, argv,
            "length", DEFAULT_LEN);


    length += length%8;
    maxWrite += maxWrite%8;

    ASSERT(maxWrite);
    ASSERT(length);

    // Number per output.
    num = maxWrite/8;

    return 0; // success
}


int start(uint32_t numInPorts, uint32_t numOutPorts) {

    ASSERT(numInPorts == 0);
    ASSERT(numOutPorts);
    DSPEW("%" PRIu32 " inputs and  %" PRIu32 " outputs",
            numInPorts, numOutPorts);

    for(uint32_t i=0; i<numOutPorts; ++i)
        qsCreateOutputBuffer(i, maxWrite);

    count = 0;
    total = 0;

    return 0; // success
}


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInputs, uint32_t numOutputs) {

    // We'll assume there is no input data.
    ASSERT(numInputs == 0);
    ASSERT(numOutputs);

    size_t n = num;
    total += n*8;
    if(total > length) {
        n -= (total - length)/8;
        total = length;
    }

    for(uint32_t i=0; i<numOutputs; ++i) {
        uint64_t *buf = qsGetOutputBuffer(0, maxWrite, maxWrite);
        for(size_t j=0; j<n; ++j)
            buf[j] = count++;
        qsOutput(i, maxWrite);
    }


DSPEW("next count=%" PRIu64, count);

    if(total == length)
        return 1; // we are finished

    return 0;
}
