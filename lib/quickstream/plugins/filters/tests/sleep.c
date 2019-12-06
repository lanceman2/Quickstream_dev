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
        "      --maxRead BYTES       default value %zu\n"
        "\n"
        "      --maxWrite BYTES      default value %zu\n"
        "\n"
        "\n    --period SEC         default value %lf seconds\n"
        "\n"
        "\n",
        QS_DEFAULT_MAXINPUT,
        QS_DEFAULT_MAXWRITE,
        seconds
        );
}


static size_t maxRead, maxWrite;


int construct(int argc, const char **argv) {

    DSPEW();

    maxRead = qsOptsGetSizeT(argc, argv,
            "maxRead", QS_DEFAULT_MAXINPUT);
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
    ASSERT(numInPorts == numOutPorts, "");


    DSPEW("BASE_FILE=%s", __BASE_FILE__);

    DSPEW("count=%d   %" PRIu32 " inputs and  %" PRIu32 " outputs",
            count++, numInPorts, numOutPorts);

    // We needed a start() to check for this error.
    if(!numInPorts || !numOutPorts) {
        ERROR("There must be 1 or more inputs and 1 or more outputs.\n");
        return 1;
    }

    size_t lens[numOutPorts];
    for(uint32_t i=0;i<numInPorts;++i)
        lens[i] = maxRead;
    qsSetInputMax(lens);

    uint32_t ports[2];
    ports[1] = QS_ARRAYTERM;
    for(uint32_t i=0;i<numInPorts;++i) {
        ports[0] = i;
        qsBufferCreate(maxWrite, ports);
    }

    return 0; // success
}


int input(const void *buffers[], const size_t lens_in[],
        const bool isFlushing[],
        uint32_t numInPorts, uint32_t numOutPorts) {

    // The filter module's stupid action, sleep.
    if(usecs)
        usleep(usecs);

    qsAdvanceInputs(lens_in);

    for(uint32_t i=0; i<numInPorts; ++i)
        memcpy(qsGetBuffer(i, lens_in[i]), buffers[i], lens_in[i]);

    qsOutputs(lens_in);

    return 0; // success
}
