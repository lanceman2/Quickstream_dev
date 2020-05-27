#include <stdio.h>
#include <time.h>

#include "../../../../include/quickstream/app.h"
#include "../../../../include/quickstream/controller.h"
#include "../../../../include/quickstream/parameter.h"
#include "../../../../include/quickstream/filter.h"
#include "../../../../lib/debug.h"



void help(FILE *f) {
    fprintf(f,
"   Usage: bytesRate\n"
"\n"
"  Measure average bytes per second.\n"
"\n"
"  At stream flow start his module looks for parameters in each\n"
"  filter with the parameter name \"bytesIn#\", \"bytesInTotal\",\n"
"  \"bytesOut#\", and \"bytesOutTotal\" and type QsUint64 and creates\n"
"  corresponding parameters \"bytesInRate#\", \"bytesInTotalRate\",\n"
"  \"bytesOut#Rate\", and \"bytesOutTotalRate\" of type QsDouble,\n"
"  where the # stands for input and output port numbers.\n"
"\n"
"  Controller module bytesCounter can be used to make the needed\n"
"  parameters: \"bytesIn#\", \"bytesInTotal\", \"bytesOut#\", and\n"
"  \"bytesOutTotal\".  This module builds parameter using the\n"
"  from bytesCounter parameters.\n"
"\n"
"  Currently this just uses simple 2 point averaging.  More options\n"
"  are needed.\n"
"\n"
"\n"
"                      OPTIONS\n"
"\n"
"\n"
"     --period SECONDS   set the period to average over.  The default averages\n"
"                        at every filter input.  Longer period may give smoother\n"
"                        and more consistent parameter values.\n"
"\n"
"\n"
"     --coarse           use a coarse timer.  Uses the CLOCK_MONOTONIC_COARSE\n"
"                        flag for clock_gettime().  This can give better\n"
"                        stream performance than the default, CLOCK_MONOTONIC.\n"
"                        \"A faster but less precise version of CLOCK_MONOTONIC.\"\n"
"\n");
}


static double period = 0;

static clockid_t clockType = CLOCK_MONOTONIC;


struct BytesRate {

    struct timespec t;

    uint64_t bytes;
};



static int
GetBytesCountCallback(uint64_t *bytes, struct QsStream *s, const char *filterName,
        const char *pName,
        enum QsParameterType type, struct BytesRate *br) {

    DSPEW("%s:%s %" PRIu64 " bytes", filterName, pName, *bytes);

    return 0;
}





int construct(int argc, const char **argv) {

    period = qsOptsGetDouble(argc, argv, "period", 0.0);

    clockType = qsOptsGetBool(argc, argv, "coarse")?CLOCK_MONOTONIC_COARSE:CLOCK_MONOTONIC;

    return 0; // success
}


int preStart(struct QsStream *s, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs) {

    WARN("filter \"%s\"", qsFilterName(f));

    // A regular expression to match for bytes count for input and
    // outputs:
    const char *regex = "^bytes(In|Out)([0-9]+|Total)$";

    qsParameterGet(s, qsFilterName(f), regex, QsUint64,
            (int (*)(
                const void *value, void *streamOrApp,
                const char *ownerName, const char *pName, 
                enum QsParameterType type, void *userData))
            GetBytesCountCallback,
            0 /*userData*/, QS_PNAME_REGEX);

    return 0;
}


int postStop(struct QsStream *s, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs) {

    WARN("filter \"%s\"", qsFilterName(f));

    return 0;
}
