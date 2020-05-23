#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "../../../../../include/quickstream/parameter.h"
#include "../../../../../include/quickstream/filter.h"
#include "../../../../../lib/debug.h"



void help(FILE *f) {

    fprintf(f,
"  Usage: tests/parameter [  ]\n"
"\n"
"This filter is just for testing the qsParameter API.\n"
"\n"
"  See tests/340_parameter.\n"
"\n"
        );
}


#define NUM  ((size_t) 4)


static const char *filterName = 0;
static double parameters[NUM];


static
int setParameterCallback(struct QsParameter *p,
        double *value, const char *pName,
        double *pValue) {

    NOTICE("Changing parameter \"%s:%s\" from %lg to %lg",
            filterName, pName, *pValue, *value);
    *pValue = *value;

    qsParameterPush(pName, pValue);

    return 0;
}



int construct(int argc, const char **argv) {

    DSPEW();

    filterName = qsGetFilterName();

    for(size_t i=0; i<NUM; ++i) {
        char pName[16];
        snprintf(pName, 16, "par %zu", i);
        parameters[i] = i + 0.0456;
        qsParameterCreate(pName, QsDouble,
                (int (*)(struct QsParameter *, void *,
                         const char *, void *)) setParameterCallback,
                0, parameters + i);
    }

    return 0; // success
}


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInputs, uint32_t numOutputs) {

    return 0;
}
