#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "../../../../../include/quickstream/filter.h"
#include "../../../../../lib/debug.h"



void help(FILE *f) {
    fprintf(f,
"  Usage: tests/copy [ --maxWrite BYTES --sleep SECS ]\n"
"\n"
"A test filter module that copies each input to each output in order.\n"
"If they are more inputs than outputs than the last output gets the\n"
"reminder of the inputs copied to it.  If there are more outputs than\n"
"inputs than the reminder outputs get no data written to them.\n"
"\n"
"                       OPTIONS\n"
"\n"
"      --maxWrite BYTES   default value %zu\n"
"\n"
"      --sleep SECS       sleep SECS seconds in each input() call.\n"
"                         By default it does not sleep.\n"   
"\n"
"\n",
        QS_DEFAULTMAXWRITE);
}


static size_t maxWrite;

static struct timespec t = { 0, 0 };
static bool doSleep = false;


int construct(int argc, const char **argv) {

    DSPEW();

    maxWrite = qsOptsGetSizeT(argc, argv,
            "maxWrite", QS_DEFAULTMAXWRITE);

    double sleepT = qsOptsGetDouble(argc, argv,
            "sleep", 0);

    if(sleepT) {

        t.tv_sec = sleepT;
        t.tv_nsec = (sleepT - t.tv_sec) * 1000000000;
        doSleep = true;

        DSPEW("Filter \"%s\" will sleep %lg "
                "seconds in every input() call.",
                qsGetFilterName(), sleepT);
    }
  
    return 0; // success
}


int start(uint32_t numInPorts, uint32_t numOutPorts) {

    DSPEW("%" PRIu32 " inputs and %" PRIu32 " outputs",
            numInPorts, numOutPorts);

    ASSERT(numInPorts);
    ASSERT(numOutPorts);

    for(uint32_t i=0; i<numOutPorts; ++i)
        qsCreateOutputBuffer(i, maxWrite);

    return 0; // success
}


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInPorts, uint32_t numOutPorts) {

    uint32_t outPortNum = 0;

    for(uint32_t i=0; i<numInPorts; ++i) {
        size_t len = lens[i];
        if(len > maxWrite)
            len = maxWrite;
        memcpy(qsGetOutputBuffer(outPortNum, len, len), buffers[i], len);
        qsOutput(outPortNum, len);
        if(outPortNum + 1 < numOutPorts)
            ++outPortNum;

        qsAdvanceInput(i, len);
    }

    if(doSleep)
        nanosleep(&t, 0);

    return 0; // success
}
