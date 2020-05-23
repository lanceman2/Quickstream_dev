// This controller module is part of a test in
// tests/372_controller_parameter
//
// If you change this file you'll likely brake that test.  If you need to
// edit this, make sure that that you also fix
// tests/372_controller_parameter, without changing the nature of that
// test.


#include <stdio.h>

#include "../../../../../include/quickstream/app.h"
#include "../../../../../include/quickstream/parameter.h"
#include "../../../../../include/quickstream/controller.h"
#include "../../../../../include/quickstream/filter.h"
#include "../../../../../lib/debug.h"



void help(FILE *f) {
    fprintf(f,
"   Usage: tests/parameter\n"
"\n"
" A test controller module that makes and tests Parameters.\n"
"\n"
"\n");
}

static
uint64_t count = 0;


static
int setCount(struct QsParameter *p,
        void *value, const char *pName, void *userData) {

    

    return 0;
}

static
int PostInputCB(
            struct QsFilter *filter,
            const size_t lenIn[],
            const size_t lenOut[],
            const bool isFlushing[],
            uint32_t numInputs, uint32_t numOutputs,
            void *userData) {

    if(numInputs)
        DSPEW("-------------PostInputCB(filter=\"%s\","
                "inputLen[0]=%zu,,)---------------",
                qsFilterName(filter), numInputs?lenIn[0]:0);
    if(numOutputs)
        DSPEW("-------------PostInputCB(filter=\"%s\","
                "outputLen[0]=%zu,,)---------------",
                qsFilterName(filter), numOutputs?lenOut[0]:0);

    return 0;
}


static
int AddFilter(struct QsStream *s, struct QsFilter *filter,
        void *userData) {

    DSPEW("Adding filter \"%s\"", qsFilterName(filter));

    qsAddPostFilterInput(filter, PostInputCB, 0);
 
    return 0;
}




int construct(int argc, const char **argv) {

    DSPEW("in construct()");

    qsParameterCreate("count", QsUint64, setCount, 0, 0);

    printf("%s()\n", __func__);
    return 0; // success
}

int preStart(struct QsStream *stream, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs) {

    DSPEW("filter=\"%s\"", qsFilterName(f));
    count = 0;
    printf("%s()\n", __func__);

    qsStreamForEachFilter(0, AddFilter, 0);

    return 1; // stop calling for this stream run.
}

int postStart(struct QsStream *stream, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs) {

    DSPEW("filter=\"%s\"", qsFilterName(f));
    printf("%s()\n", __func__);
    return 0;
}

int preStop(struct QsStream *stream, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs) {

    DSPEW("filter=\"%s\"", qsFilterName(f));
    printf("%s()\n", __func__);
    return 0;
}

int postStop(struct QsStream *stream, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs) {

    DSPEW("filter=\"%s\"", qsFilterName(f));
    printf("%s()\n", __func__);
    return 0;
}

int destroy(void) {

    DSPEW();
    printf("%s()\n", __func__);
    return 0; // success
}
