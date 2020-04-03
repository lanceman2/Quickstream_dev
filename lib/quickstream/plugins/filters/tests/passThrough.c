#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "../../../../../include/quickstream/filter.h"
#include "../../../../../lib/debug.h"



void help(FILE *f) {

    fprintf(f,
"  Usage: tests/passThrough [ --maxWrite BYTES --sleep SECS ]\n"
"\n"
"This filter reads N input and writes N outputs via a pass-through buffer.\n"
"\n"
"                  OPTIONS\n"
"\n"
"\n"
"   --maxWrite BYTES  default value %zu.  This is the number of\n"
"                     bytes read and written for each input() call.\n"
"\n"
"   --sleep SECS      sleep SECS seconds in each input() call.\n"
"                     By default it does not sleep.\n"   
"\n"
"\n",
        QS_DEFAULTMAXWRITE);
}


static size_t maxWrite;
static struct timespec t = { 0, 0 };
const char *filterName = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
double _sleepT = 0;

double sleepT = 0;


int setSleepCallback(void *value) {

    DSPEW("sleep time set to %lg seconds", *(double *) value);

    ASSERT(pthread_mutex_lock(&mutex) == 0);
    _sleepT = *(double *) value;
    ASSERT(pthread_mutex_unlock(&mutex) == 0);

    return 0;
}



int construct(int argc, const char **argv) {

    DSPEW();

    maxWrite = qsOptsGetSizeT(argc, argv,
            "maxWrite", QS_DEFAULTMAXWRITE);

    filterName = qsGetFilterName();

    ASSERT(maxWrite);

    _sleepT = sleepT = qsOptsGetDouble(argc, argv,
            "sleep", 0);

    if(sleepT) {

        t.tv_sec = sleepT;
        t.tv_nsec = (sleepT - t.tv_sec) * 1000000000;

        DSPEW("Filter \"%s\" will sleep %lg "
                "seconds in every input() call.",
                filterName, sleepT);
    }

    qsParameterCreate("sleep", QsDouble, setSleepCallback);

    return 0; // success
}


int start(uint32_t numInputs, uint32_t numOutputs) {

    ASSERT(numInputs);
    ASSERT(numInputs == numOutputs);

    for(uint32_t i=0; i<numInputs; ++i)
        if(qsCreatePassThroughBuffer(i, i, maxWrite))
            return 1; // fail

    return 0; // success
}


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInputs, uint32_t numOutputs) {

    DASSERT(lens);
    DASSERT(lens[0]);

    //DSPEW("lens[0]=%zu", lens[0]);


    if(sleepT != _sleepT) {

        ASSERT(pthread_mutex_lock(&mutex) == 0);
        sleepT = _sleepT;
        t.tv_sec = sleepT;
        t.tv_nsec = (sleepT - t.tv_sec) * 1000000000;
        ASSERT(pthread_mutex_unlock(&mutex) == 0);
    }

    if(sleepT)
        nanosleep(&t, 0);

    for(uint32_t i=0; i<numInputs; ++i) {
        size_t len = lens[i];
        if(len > maxWrite)
            len = maxWrite;
        else if(len == 0)
            continue;

        char *in = buffers[i];
        char *out = qsGetOutputBuffer(i, len, len);

        ASSERT(in == out);

        qsAdvanceInput(i, len);
        qsOutput(i, len);
    }

    return 0;
}
