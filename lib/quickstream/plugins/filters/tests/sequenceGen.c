#include "../../../../../include/quickstream/filter.h"
#include "../../../../../lib/debug.h"

#include "Sequence.h"

// This is the default total output length to all output channels
// for a given stream run.
#define DEFAULT_TOTAL_LENGTH   ((size_t) 800000)

void help(FILE *f) {

    fprintf(f,
"This filter is a source.  This filter writes a fixed pseudo-random\n"
"sequence of one byte ascii characters in hex to all outputs.  Each\n"
"output has a different sequence which comes from seeding the\n"
"randomString_init() function with the output port numbers, 0, 1, 2,\n"
"and etc at each start().\n"
"\n"
"                  OPTIONS\n"
"\n"
"\n"
"    --maxWrite BYTES    default value %zu.  This is the number of\n"
"                        bytes read and written for each input() call.\n"
"\n"
"    --length LEN        Write LEN bytes total and than finish.\n"
"                        The default LEN is %zu.\n"
"\n"
"\n",
QS_DEFAULTMAXWRITE, DEFAULT_TOTAL_LENGTH);
}


static size_t maxWrite;
static size_t totalOut, count;
static struct RandomString *rs;


int construct(int argc, const char **argv) {

    DSPEW();

    maxWrite = qsOptsGetSizeT(argc, argv,
            "maxWrite", QS_DEFAULTMAXWRITE);
    totalOut = qsOptsGetSizeT(argc, argv,
            "length", DEFAULT_TOTAL_LENGTH);

    ASSERT(maxWrite);
    ASSERT(totalOut);

    return 0; // success
}


int start(uint32_t numInputs, uint32_t numOutputs) {

    ASSERT(numInputs == 0);
    ASSERT(numOutputs);

    DSPEW("%" PRIu32 " outputs", numOutputs);

    rs = calloc(numOutputs, sizeof(*rs));
    ASSERT(rs, "calloc(%" PRIu32 ",%zu) failed",
            numOutputs, sizeof(*rs));

    for(uint32_t i=0; i<numOutputs; ++i) {
        qsCreateOutputBuffer(i, maxWrite);
        // Initialize the random string generator.
        randomString_init(rs + i, i/*seed*/);
    }


    count = 0;

    return 0; // success
}


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInputs, uint32_t numOutputs) {

    DASSERT(numInputs == 0);
    DASSERT(numOutputs);

    int ret = 0;

    size_t len = maxWrite;
    if(count + len <= totalOut)
        count += len;
    else {
        len = totalOut - count;
        ret = 1; // Last time calling input().
    }

    if(len == 0) return 1; // We're done.

    for(uint32_t i=0; i<numOutputs; ++i) {
        void *out = qsGetOutputBuffer(i, len, len);
        randomString_get(rs + i, len, out);
        qsOutput(i, len);
    }

    return ret;
}


int stop(uint32_t numInputs, uint32_t numOutputs) {

    DASSERT(numInputs == 0);
    DASSERT(numOutputs);
    DASSERT(rs);

    free(rs);
    rs = 0;

    return 0;
}
