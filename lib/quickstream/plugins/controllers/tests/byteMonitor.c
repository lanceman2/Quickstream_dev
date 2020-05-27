#include <stdio.h>

#include "../../../../../include/quickstream/app.h"
#include "../../../../../include/quickstream/controller.h"
#include "../../../../../include/quickstream/parameter.h"
#include "../../../../../include/quickstream/filter.h"
#include "../../../../../lib/debug.h"


#define DEFAULT_PERIOD    ((double) 0.4) // seconds


void help(FILE *f) {
    fprintf(f,
"   Usage: byteMonitor\n"
"\n"
"  A controller module that prints the bytes counters to stdout.\n"
"  At stream flow start his module looks for parameters in each\n"
"  filter with the parameter name \"bytesIn#\", \"bytesInTotal\",\n"
"  \"bytesOut#\", and \"bytesOutTotal\" and type QsUint64 and creates\n"
"  corresponding parameters \"bytesInRate#\", \"bytesInTotalRate\",\n"
"  \"bytesOut#Rate\", and \"bytesOutTotalRate\" of type QsDouble,\n"
"  where the # stands for input and output port numbers.\n"
"\n"
"  Controller module bytesCounter can be used to make the needed\n"
"  parameters: \"bytesIn#\", \"bytesInTotal\", \"bytesOut#\", and\n"
"  \"bytesOutTotal\".\n"
"\n"
"\n"
"                      OPTIONS\n"
"\n"
"\n"
"     --period SECONDS   update every SECONDS seconds.   The default is\n"
"                        %lg seconds.\n"
"\n",
    DEFAULT_PERIOD);
}


static double period;


int construct(int argc, const char **argv) {

    period = qsOptsGetDouble(argc, argv, "period", DEFAULT_PERIOD);

    return 0; // success
}


int postStart(struct QsStream *s, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs) {

    WARN("filter \"%s\"", qsFilterName(f));

    return 0;
}


int postStop(struct QsStream *s, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs) {

    WARN("filter \"%s\"", qsFilterName(f));

    return 0;
}
