#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define QS_FILTER_NAME_CODE
#include "../../../../../include/qsfilter.h"
#include "../../../../../lib/debug.h"

#ifdef SPEW_LEVEL_DEBUG
static int count = 0;
#endif


//static unsigned int usecs = 200000; // 1 micro second = 1s/1,000,000
static unsigned int usecs = 0; // 1 micro second = 1s/1,000,000
static double seconds = 0;

void help(FILE *f) {
    fprintf(f,
        "test filter module that copies all input to each output and sleeps\n"
        "\n"
        "                       OPTIONS\n"
        "\n"
        "      --maxWrite BYTES      default value %zu\n"
        "\n"
        "\n    --period SEC         default value %lf seconds\n"
        "\n"
        "\n",
        QS_DEFAULT_MAXWRITE,
        seconds
        );
}


static size_t maxWrite;


int construct(int argc, const char **argv) {

    DSPEW();

    maxWrite = qsOptsGetSizeT(argc, argv,
            "maxWrite", QS_DEFAULT_MAXWRITE);
    seconds = qsOptsGetSizeT(argc, argv,
            "period", QS_DEFAULT_MAXWRITE);

    usecs = seconds * 1000000;

    if(usecs < 1.0e-7)
        usecs = 0;

    return 0; // success
}


int start(uint32_t numInPorts, uint32_t numOutPorts) {

    ASSERT(numInPorts, "");
    ASSERT(numOutPorts, "");

    DSPEW("count=%d   %" PRIu32 " inputs and  %" PRIu32 " outputs",
            count++, numInPorts, numOutPorts);

    return 0; // success
}


int stop(uint32_t numInPorts, uint32_t numOutPorts) {

    DSPEW("count=%d   %" PRIu32 " inputs and  %" PRIu32 " outputs",
            count++, numInPorts, numOutPorts);

    return 0; // success
}


int input(const void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInPorts, uint32_t numOutPorts) {

    if(usecs)
        // The filter module's stupid action, sleep.
        usleep(usecs);

    uint32_t outPortNum = 0;

    for(uint32_t i=0; i<numInPorts; ++i) {
        size_t len = lens[i];
        if(len > maxWrite)
            len = maxWrite;

        memcpy(qsGetOutputBuffer(outPortNum, len, len),
                buffers[i], len);
        qsOutput(outPortNum, len);
        if(outPortNum + 1 < numOutPorts)
            ++outPortNum;

        qsAdvanceInput(i, len);
    }

    return 0; // success
}
