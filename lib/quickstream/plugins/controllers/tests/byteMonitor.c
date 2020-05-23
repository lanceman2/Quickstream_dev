#include <stdio.h>

#include "../../../../../include/quickstream/app.h"
#include "../../../../../include/quickstream/controller.h"
#include "../../../../../include/quickstream/parameter.h"
#include "../../../../../lib/debug.h"


void help(FILE *f) {
    fprintf(f,
"   Usage: byteMonitor\n"
"\n"
" A controller module that prints the bytes counters to stdout.\n"
"\n"
"\n");
}




int postStop(struct QsStream *s, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs) {

    INFO("filter \"%s\"", qsFilterName(f));

    return 0;
}

