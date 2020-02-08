#include "../../../../../include/quickstream/filter.h"
#include "../../../../../lib/debug.h"

#include "Sequence.h"


void help(FILE *f) {

    fprintf(f,
        "The filter reads and checks input that came from sequenceGen.\n"
        "The filter reads and number of inputs and writes out what is reads\n"
        "in.\n"
        "\n"
        "\n"
        "                  OPTIONS\n"
        "\n"
        "\n"
        "    --maxWrite BYTES  default value %zu.  This is the number of\n"
        "                      bytes read and written for each input() call.\n"
        "\n",
        QS_DEFAULTMAXWRITE);
}


static size_t maxWrite;
static char **compare;
static struct RandomString *rs;
static const char *filterName;


int construct(int argc, const char **argv) {

    DSPEW();

    maxWrite = qsOptsGetSizeT(argc, argv,
            "maxWrite", QS_DEFAULTMAXWRITE);

    ASSERT(maxWrite);

    filterName = qsGetFilterName();

    return 0; // success
}


int start(uint32_t numInputs, uint32_t numOutputs) {

    ASSERT(numInputs);

    DSPEW("%" PRIu32 " inputs  %" PRIu32 " outputs",
            numInputs, numOutputs);

    if(numInputs > numOutputs) {
        compare = calloc(numInputs - numOutputs, sizeof(*compare));
        ASSERT(compare, "calloc(%" PRIu32 ",%zu) failed",
                numInputs - numOutputs, sizeof(*compare));
    }

    for(uint32_t i=0; i<numInputs - numOutputs; ++i) {
        compare[i] = malloc(maxWrite + 1);
        ASSERT(compare[i], "malloc(%zu) failed", maxWrite + 1);
        qsCreateOutputBuffer(i, maxWrite);
    }


    rs = calloc(numInputs, sizeof(*rs));
    ASSERT(rs, "calloc(%" PRIu32 ",%zu) failed",
            numInputs, sizeof(*rs));

    for(uint32_t i=0; i<numInputs; ++i) {
        // Initialize the random string generator.
        randomString_init(rs + i, i/*seed*/);
    }

    return 0; // success
}


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInputs, uint32_t numOutputs) {

    for(uint32_t i=0; i<numInputs; ++i) {

        size_t len = lens[i];
        if(len > maxWrite)
            len = maxWrite;
        else if(len == 0)
            continue;

        char *in = buffers[i];

        if(i < numOutputs) {

            char *out = qsGetOutputBuffer(i, len, len);
            randomString_get(rs + i, len, out);
            for(size_t j=0; j<len; ++j)
                ASSERT(out[j] == in[j],
                        "%s Miss-match on input channel %" PRIu32,
                        filterName, i);
            qsOutput(i, len);

        } else {

            // This is no corresponding output for this input port.
            char *out = compare[i - numOutputs];
            DSPEW("SHIT I GOT TO GO out=%s", out);

        }
    }


    return 0;
}


int stop(uint32_t numInputs, uint32_t numOutputs) {

    DASSERT(rs);
    free(rs);
    rs = 0;

    for(uint32_t i=0; i<numInputs - numOutputs; ++i) {
        DASSERT(compare[i]);
        free(compare[i]);
        compare[i] = 0;
    }

    if(numInputs > numOutputs) {
        DASSERT(compare);
        free(compare);
        compare = 0;
    }

    return 0;
}
