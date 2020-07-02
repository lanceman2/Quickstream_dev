#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "../../../../include/quickstream/app.h"
#include "../../../../include/quickstream/controller.h"
#include "../../../../include/quickstream/parameter.h"
#include "../../../../include/quickstream/filter.h"
#include "../../../debug.h"


#define DEFAULT_PERIOD  ((double) 0.01)


static clockid_t clockType = CLOCK_MONOTONIC_COARSE;
static double period = DEFAULT_PERIOD;



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
"  --not-coarse   use a fine timer; use the CLOCK_MONOTONIC flag\n"
"                 for clock_gettime().  CLOCK_MONOTONIC_COARSE is\n"
"                 a faster but less precise version of\n"
"                 CLOCK_MONOTONIC.   CLOCK_MONOTONIC_COARSE is\n"
"                 used by default.  See 'man 2 clock_gettime'.\n"
"                 Using CLOCK_MONOTONIC could hurt your filter\n"
"                 stream performance.\n"
"\n"
"\n"
"  --report-period SECONDS  Set the time between reports.  The\n"
"                           default report period is %lg seconds.\n"
"                 If the stream runs in a time less than this\n"
"                 period then you will get no measurements.\n"
"\n",
    DEFAULT_PERIOD);
}



struct BytesRate {

    struct timespec prevT;

    uint64_t bytes;

    struct QsParameter *parameter;
};


// 
static int
GetBytesCountCallback(uint64_t *bytes, struct QsStream *s,
        const char *filterName,
        const char *pName,
        enum QsParameterType type, struct BytesRate *br) {

    //WARN("%s:%s %" PRIu64 " bytes", filterName, pName, *bytes);

    struct timespec t;
    // TODO: clock_gettime() may be too expensive to do this often, but
    // maybe the CLOCK_MONOTONIC_COARSE flag will ease the use of system
    // resources.
    //
    clock_gettime(clockType, &t);

    if(br->prevT.tv_sec == 0) {
        // The previous time was not initialized.
        br->bytes = *bytes;
        // We wait until the next call to get a value.
        br->prevT.tv_sec = t.tv_sec;
        br->prevT.tv_nsec = t.tv_nsec;
        return 0;
    }

    double bytesRate = t.tv_sec - br->prevT.tv_sec +
        (1.0e-9)*(t.tv_nsec - br->prevT.tv_nsec);

    if(bytesRate < period)
        // Maybe it's possible that the clock resolution is not enough
        // to measure a time difference; if so we wait for the next or
        // other later call.
        return 0;

    bytesRate = (*bytes - br->bytes)/bytesRate;

    qsParameterPushByPointer(br->parameter, &bytesRate);

    //printf("%s:%sRate = %lg bytes/s\n", filterName, pName, bytesRate);

    // Save the last measurements
    br->bytes = *bytes;
    br->prevT.tv_sec = t.tv_sec;
    br->prevT.tv_nsec = t.tv_nsec;

    return 0;
}



int construct(int argc, const char **argv) {

    clockType = qsOptsGetBool(argc, argv, "not-coarse")?
        CLOCK_MONOTONIC:CLOCK_MONOTONIC_COARSE;
    period = qsOptsGetDouble(argc, argv, "report-period", DEFAULT_PERIOD);

    return 0; // success
}


static void FreeByteRate(const char *pName, struct BytesRate *br) {

    DASSERT(br);
    free(br);
};


static int CreateByteRateParameter(struct QsStream *s,
            const char *filterName, const char *pName_in,
            enum QsParameterType type, struct QsFilter *f) {

    char pName[24];
    snprintf(pName, 24, "%sRate", pName_in);

    struct BytesRate *br = calloc(1, sizeof(*br));
    ASSERT(br, "calloc(1,%zu) failed", sizeof(*br));

    br->parameter = qsParameterCreateForFilter(f,
        pName, QsDouble,
        0/*setCallback() is not needed*/,
        (void (*)(const char *pName, void *userData)) FreeByteRate,
        br);
    ASSERT(br->parameter, "Can't create parameter %s:%s", filterName, pName);

    qsParameterGet(s, filterName, pName_in, QsUint64,
            (int (*)(
                const void *value, void *streamOrApp,
                const char *ownerName, const char *pName,
                enum QsParameterType type, void *userData))
            GetBytesCountCallback,
            br /*userData*/, 0);

    return 0;
}


int postStart(struct QsStream *s, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs) {

    // A regular expression to match for bytes count for input and
    // outputs:
    const char *regex = "^bytes(In|Out)([0-9]+|Total)$";

    qsParameterForEach(0, s, qsFilterName(f), regex, QsUint64,
            (int (*)(
            struct QsStream *stream,
            const char *filterName, const char *pName,
            enum QsParameterType type, void *userData))
            CreateByteRateParameter,
            f, QS_PNAME_REGEX);

    return 0;
}


int postStop(struct QsStream *s, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs) {

    DSPEW("filter \"%s\"", qsFilterName(f));

    // We destroy all the parameter that this module created.  This is
    // required if the stream is allowed re-configure before the next
    // start.

    // A regular expression to match for bytes rate for input and
    // outputs:
    qsParameterDestroyForFilter(f, "^bytes(In|Out)([0-9]+|Total)Rate$",
            QS_PNAME_REGEX);

    return 0;
}
