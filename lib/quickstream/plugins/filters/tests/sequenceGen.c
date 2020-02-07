#include "../../../../../include/quickstream/filter.h"
#include "../../../../../lib/debug.h"

#include "Sequence.h"

// This is the default total output length to all output channels
// for a given stream run.
#define DEFAULT_TOTAL_LENGTH   ((size_t) 800000)

void help(FILE *f) {

    fprintf(f,
        "This filter is a source.  This filter writes a sequence of 64 bit\n"
        "numbers to all outputs.  The sequence is explained in:\n"
        "\n"
        "       https://en.wikipedia.org/wiki/Maximum_length_sequence\n"
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
        "\n", QS_DEFAULTMAXWRITE, DEFAULT_TOTAL_LENGTH);
}


static size_t maxWrite;
static size_t totalLength, num, total;
static uint64_t count;


#if 0
static inline
uint64_t GetNext(uint64_t num) {

    uint64_t ret = num << 1;
    return 0;

}
#endif



int construct(int argc, const char **argv) {

    DSPEW();

    maxWrite = qsOptsGetSizeT(argc, argv,
            "maxWrite", QS_DEFAULTMAXWRITE);
    totalLength = qsOptsGetSizeT(argc, argv,
            "length", DEFAULT_TOTAL_LENGTH);


    totalLength += totalLength%8;
    maxWrite += maxWrite%8;

    ASSERT(maxWrite);
    ASSERT(totalLength);

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
    if(total > totalLength) {
        n -= (total - totalLength)/8;
        total = totalLength;
    }

    for(uint32_t i=0; i<numOutputs; ++i) {
        uint64_t *buf = qsGetOutputBuffer(0, n*8, n*8);
        for(size_t j=0; j<n; ++j)
            buf[j] = count++;
        qsOutput(i, n*8);
    }


//DSPEW("next count=%" PRIu64, count);

    if(total == totalLength)
        return 1; // we are finished

    return 0;
}
