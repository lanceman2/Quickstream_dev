// quickstream controller module that add a byte counter for every filter
// input and output port.
//
// This module should be able to work with more than one stream running.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#include "../../../../include/quickstream/app.h"
#include "../../../../include/quickstream/filter.h"
#include "../../../../include/quickstream/controller.h"
#include "../../../../include/quickstream/parameter.h"
#include "../../../debug.h"



void help(FILE *f) {
    fprintf(f,
"   Usage: bytesCounter\n"
"\n"
"   A controller module that adds a bytes counter for all\n"
" filters in all streams.\n"
"\n"
"\n                OPTIONS\n"
"\n"
"   --printNoSummary    do not print a count summary to stderr.\n"
"                       By default a summary is printed to stderr\n"
"                       after each stream run.\n"
"\n"
"\n");
}

struct Counter {

    uint64_t count;
    struct QsParameter *parameter;
};


struct FilterBytesCounter {

    struct QsFilter *filter;

    // ports 0, 1, 2, 3, .. N-1, and Total
    // For just one port it's just Total at element 0.

    uint32_t numInputs;
    uint32_t numOutputs;

    // If there is just one input port then there will be one counter for
    // port 0 and the total, but a parameter for each.
    struct Counter *inputCounters; // array of counters

    // If there is just one output port then there will be one counter for
    // port 0 and the total, but a parameter for each.
    struct Counter *outputCounters; // array of counters

    struct timespec start;
};


// Related to option --printNoSummary
//
static bool printSummary = true;
// Marker to keep us from printing the stream header more than once.
static uint32_t printingStreamId = -1;


void PrintStreamHeader(const struct QsFilter *f) {



    fprintf(stderr,
        "  |******************************************************************|\n"
        "  |*****                  Stream %3" PRIu32
        "                            *****|\n"
        "  |******************************************************************|\n"
        "  |                                                                  |\n",
        qsFilterStreamId(f));
}


static inline void
PrintSummary(struct FilterBytesCounter *bc, FILE *file) {

    DASSERT(bc);
    DASSERT(bc->filter);

    const char *st =
            "  |------------------------------------------------------------------|";
    const char *line =
            "----------------------------------------------------------------";

    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);

    // Change in time in seconds since start.
    double dt = t.tv_sec - bc->start.tv_sec +
        (1.0e-9)*(t.tv_nsec - bc->start.tv_nsec);


    fprintf(file,
                "%s\n"
                "  |    Filter \"%s\"     %3.3lg seconds\n"
                "%s\n",
                st, qsFilterName(bc->filter), dt, st);

    if(bc->numInputs) {
        uint32_t i=0;
        fprintf(file,
                "    |%s|\n"
                "    |------------------------   INPUTS   ----------------------------|\n"
                "    |%s|\n"
                "    |   port_number             bytes              average bytes/sec |\n"
                "    |   -------------    ----------------------   -------------------|\n",
                line, line);
        for(; i<bc->numInputs; ++i)
            fprintf(file,
                "    |          %4" PRIu32 "      %21" PRIu64 "          %3.3lg\n",
                    i, bc->inputCounters[i].count, bc->inputCounters[i].count/dt);
        fprintf(file,
                "    |         total      %21" PRIu64 "          %3.3lg\n"
                "    |%s|\n",
                bc->inputCounters[i].count, bc->inputCounters[i].count/dt,
                line);
    } else
        fprintf(file,
                "    |%s|\n"
                "    |----------------------   NO INPUTS   ---------------------------|\n",
                line);

    if(bc->numOutputs) {
        uint32_t i=0;
        fprintf(file,
                "    |%s|\n"
                "    |----------------------    OUTPUTS    ---------------------------|\n"
                "    |%s|\n"
                "    |    port_number             bytes             average bytes/sec |\n"
                "    |   -------------    ----------------------   -------------------|\n",
                line, line);
        for(; i<bc->numOutputs; ++i)
            fprintf(file,
                "    |          %4" PRIu32 "      %21" PRIu64 "          %3.3lg\n",
                    i, bc->outputCounters[i].count, bc->outputCounters[i].count/dt);
        fprintf(file,
                "    |         total      %21" PRIu64 "          %3.3lg\n"
                "    |%s|\n",
                bc->outputCounters[i].count, bc->outputCounters[i].count/dt,
                line);
    } else
        fprintf(file,
                "    |%s|\n"
                "    |--------------------    NO OUTPUTS    --------------------------|\n",
                line);

    fprintf(file,
                "    |%s|\n",
                line);

}


static void
CleanFilterByteCounter(const char *pName,
        struct FilterBytesCounter *bc) {

    DASSERT(bc);
    DASSERT(bc->filter);

    //DSPEW("Cleaning up filter parameter \"%s:%s\"",
    //        qsFilterName(bc->filter), pName);

    if(printingStreamId == qsFilterStreamId(bc->filter))
        PrintSummary(bc, stderr);


    if(bc->numInputs) {
#ifdef DEBUG
        DASSERT(bc->inputCounters);

        // Zeroing the counters may help in debugging.  In general zeroing
        // memory before freeing it can help debugging, so long as we have
        // all the DASSERT() checks in the code.
        memset(bc->inputCounters, 0,
                (bc->numInputs+1)*sizeof(*bc->inputCounters));
#endif
        free(bc->inputCounters);
    }
#ifdef DEBUG
    else
        DASSERT(bc->inputCounters == 0);
#endif


    if(bc->numOutputs) {
#ifdef DEBUG
        DASSERT(bc->outputCounters);

        // Zeroing the counters may help in debugging.
        memset(bc->outputCounters, 0,
                (bc->numOutputs+1)*sizeof(*bc->outputCounters));
#endif
        free(bc->outputCounters);
    }
#ifdef DEBUG
    else
        DASSERT(bc->outputCounters == 0);
#endif

#ifdef DEBUG
    memset(bc, 0, sizeof(*bc));
#endif

    free(bc);
}


int construct(int argc, const char **argv) {

    //DSPEW();

    printSummary = !qsOptsGetBool(argc, argv, "printNoSummary");

    return 0; // success
}


// This callback is called after every filter module input() call.
//
int postFilterInputCB(
            struct QsFilter *f,
            const size_t lenIn[],
            const size_t lenOut[],
            const bool isFlushing[],
            uint32_t numInputs, uint32_t numOutputs,
            struct FilterBytesCounter *bc) {

    // TODO: How will multi-threaded filter::input() functions break this?
    // Do we need a mutex for multi-threaded filter::input().
    //
    // For single threaded filter input() this will be called after the
    // filter input() in that thread call and not during, so there's no
    // multi-thread issue here.
    //
    //DSPEW("filter=\"%s\"", qsFilterName(f));

    DASSERT(numInputs == bc->numInputs);
    DASSERT(numOutputs == bc->numOutputs);

    if(numInputs) {
        // zero the total at the start of this for loop.
        bc->inputCounters[numInputs].count = 0;
        uint32_t i=0;
        for(; i<numInputs; ++i) {
            // Add to the port count and the total is the sum of all.
            bc->inputCounters[numInputs].count +=
                (bc->inputCounters[i].count +=
                    lenIn[i]);
            // Port i
            qsParameterPushByPointer(bc->inputCounters[i].parameter,
                    &(bc->inputCounters[i].count));
        }
        // Total
        qsParameterPushByPointer(bc->inputCounters[i].parameter,
                    &(bc->inputCounters[i].count));
    }

    if(numOutputs) {
        // zero the total at the start of this for loop.
        bc->outputCounters[numOutputs].count = 0;
        uint32_t i=0;
        for(; i<numOutputs; ++i) {
            // Add to the port count and the total is the sum of all.
            bc->outputCounters[numOutputs].count +=
                (bc->outputCounters[i].count +=
                    lenOut[i]);
            // Port i
            qsParameterPushByPointer(bc->outputCounters[i].parameter,
                    &(bc->outputCounters[i].count));
        }
        // Total
        qsParameterPushByPointer(bc->outputCounters[i].parameter,
                &(bc->outputCounters[i].count));
    }

    return 0;
}


int preStart(struct QsStream *s, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs) {

    printingStreamId = -1;

    DSPEW("filter=\"%s\"", qsFilterName(f));

    if(numInputs == 0 && numOutputs == 0)
        // There are no input or output bytes to count.
        return 0; // So we are done.


    // Allocate a FilterBytesCounter
    // A counter for all inputs and outputs and totals.
    struct FilterBytesCounter *bc;
    bc = calloc(1, sizeof(*bc));
    ASSERT(bc, "calloc(1,%zu) failed", sizeof(*bc));
    bc->numInputs = numInputs;
    bc->numOutputs = numOutputs;
    bc->filter = f;

    clock_gettime(CLOCK_MONOTONIC, &bc->start);

    // Allocate input counters
    if(numInputs) {
        // We require 1 more parameter for the total.
        bc->inputCounters = calloc(numInputs + 1,
                sizeof(*bc->inputCounters));
        ASSERT(bc->inputCounters, "calloc(%" PRIu32 ",%zu) failed",
                numInputs + 1, sizeof(*bc->inputCounters));
    }

    // Allocate output counters
    if(numOutputs) {
        // We require 1 more counter for the total.
        bc->outputCounters = calloc(numOutputs + 1,
                sizeof(*bc->outputCounters));
        ASSERT(bc->outputCounters, "calloc(%" PRIu32 ",%zu) failed",
                numOutputs + 1, sizeof(*bc->outputCounters));
    }

    // Make a parameter object for each input port and an input total
    if(numInputs) {
        uint32_t i=0;
        for(; i<numInputs; ++i) {
            char pName[16];
            snprintf(pName, 16, "bytesIn%" PRIu32, i);
            bc->inputCounters[i].parameter =
                qsParameterCreateForFilter(f,
                    pName, QsUint64, 
                    0 /*setCallback=0*/,
                    0 /*cleanup=0*/,
                    bc/*userData*/);
            // TODO: add a failure mode here and below like here...
        }

        // Note: we let this parameter do the cleanup of the struct
        // FilterBytesCounter for all of the parameters that we create for
        // this filter.  We'll have just one cleanup function for all
        // byte counter parameters that we make for this filter.
        bc->inputCounters[numInputs].parameter =
            qsParameterCreateForFilter(f,
                    "bytesInTotal", QsUint64, 
                    0 /*setCallback=0*/,
                    (void (*)(const char *pName, void *userData))
                        CleanFilterByteCounter/*cleanup*/,
                    bc/*userData*/);
    }

    // Make a parameter object for each output port and an output total
    if(numOutputs) {
        uint32_t i=0;
        for(; i<numOutputs; ++i) {
            char pName[16];
            snprintf(pName, 16, "bytesOut%" PRIu32, i);
            bc->outputCounters[i].parameter =
                qsParameterCreateForFilter(f,
                    pName, QsUint64, 
                    0 /*setCallback=0*/,
                    0 /*cleanup=0*/,
                    bc/*userData*/);
        }

        // Note: we let this parameter do the cleanup of the struct
        // FilterBytesCounter for all of the parameters that we create for
        // this filter.  We'll have just one cleanup function for all
        // byte counter parameters that we make for this filter.
        bc->outputCounters[numOutputs].parameter =
            qsParameterCreateForFilter(f,
                    "bytesOutTotal", QsUint64, 
                    0 /*setCallback=0*/,
                    // We have a cleanup function for this parameter
                    // if we didn't have one for the inputs above.
                    (void (*)(const char *pName, void *userData))
                        ((numInputs)?0:CleanFilterByteCounter)/*cleanup*/,
                    bc/*userData*/);
    }

    qsAddPostFilterInput(f,  (int (*)(
                struct QsFilter *filter,
                const size_t lenIn[],
                const size_t lenOut[],
                const bool isFlushing[],
                uint32_t numInputs, uint32_t numOutputs,
                void *userData)) postFilterInputCB,
            bc);

    return 0; // keep calling for all filters.
}


int postStop(struct QsStream *s, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs) {

    //WARN("filter=\"%s\"", qsFilterName(f));

    if(printSummary &&
            printingStreamId != qsFilterStreamId(f)) {
        PrintStreamHeader(f);
        printingStreamId = qsFilterStreamId(f);
    }

    // Because quickstream can re-configure filter connections between
    // stream runs, we must destroy all the parameters that we created
    // and recreate them (possibly differently) again in the next run
    // preStart().


    if(numInputs) {
        uint32_t i=0;
        for(; i<numInputs; ++i) {
            char pName[16];
            snprintf(pName, 16, "bytesIn%" PRIu32, i);
            qsParameterDestroyForFilter(f, pName, 0);
        }
        qsParameterDestroyForFilter(f,"bytesInTotal", 0);
    }

    if(numOutputs) {
        uint32_t i=0;
        for(; i<numOutputs; ++i) {
            char pName[16];
            snprintf(pName, 16, "bytesOut%" PRIu32, i);
            qsParameterDestroyForFilter(f, pName, 0);
        }
        qsParameterDestroyForFilter(f, "bytesOutTotal", 0);
    }

    return 0;
}
