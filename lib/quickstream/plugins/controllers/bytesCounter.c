// This controller module is part of a test in tests/362_controller_dummy
//
// If you change this file you'll likely brake that test.  If you need to
// edit this, make sure that that you also fix tests/362_controller_dummy,
// without changing the nature of that test.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../../include/quickstream/app.h"
#include "../../../../include/quickstream/filter.h"
#include "../../../../include/quickstream/controller.h"
#include "../../../../include/quickstream/parameter.h"
#include "../../../../lib/debug.h"

// Since this code comes with quickstream we can use the some of its'
// "internal lib" API, that is QsDictionary.
//
// Other non-quickstream developers may want a Dictionary object, but we
// don't have a pressing need to make QsDictionary part of the public
// libquickstream API.  libquickstream is a streaming framework not a
// general C/C++ utility library.  Exposing it would be bad form.
// The same can be said of debug.h.

#include <pthread.h>
#include <string.h>
#include <stdatomic.h>

#include "../../../../lib/Dictionary.h"
#include "../../../../lib/qs.h"



void help(FILE *f) {
    fprintf(f,
"   Usage: bytesCounter\n"
"\n"
" A controller module that adds a bytes counter for\n"
" all filters in all streams.\n"
"\n"
"\n                OPTIONS\n"
"\n"
"    --filter NAME0 [...]  add the bytes counter for just filters\n"
"                          with the following names NAME0 ...\n"
"                          Only the last --filter option will be used.\n"
"\n"
"\n"
"   --printNoSummary       do not print a count summary to stderr.\n"
"                          By default a summary is printed to stderr\n"
"                          after each stream run.\n"
"\n"
"\n");
}


struct FilterBytesCounter {

    struct QsFilter *filter;

    // ports 0, 1, 2, 3, .. N-1, and Total
    // For just one port it's just Total at element 0.

    uint32_t numInputs;
    uint32_t numOutputs;

    // If there is just one input port then there will be one counter for
    // port 0 and the total.
    uint64_t *inputCounts; // array of counters
        
    // If there is just one output port then there will be one counter for
    // port 0 and the total.
    uint64_t *outputCounts; // array of counters
};

static bool printSummary = true;


static inline void
PrintSummary(struct FilterBytesCounter *bc, FILE *file) {

    DASSERT(bc);
    DASSERT(bc->filter);

    const char *st =
            "  |---------------------------------------------|";
    const char *line =
            "-------------------------------------------";


    fprintf(file,
                "%s\n"
                "  |    Filter \"%s\"\n"
                "%s\n",
                st, qsFilterName(bc->filter), st);

    if(bc->numInputs) {
        uint32_t i=0;
        fprintf(file,
                "    |%s|\n"
                "    |---------------   INPUTS   ----------------|\n"
                "    |%s|\n"
                "    |   port_number             bytes           |\n"
                "    |   -------------    -----------------------|\n",
                line, line);
        for(; i<bc->numInputs; ++i)
            fprintf(file,
                "    |          %4" PRIu32 "      %21" PRIu64 "  |\n",
                    i, bc->inputCounts[i]);
        if(bc->numInputs == 1) i = 0;
        fprintf(file,
                "    |         total      %21" PRIu64 "  |\n"
                "    |%s|\n",
                bc->inputCounts[i],
                line);
    } else
        fprintf(file,
                "    |%s|\n"
                "    |-------------   NO INPUTS   ---------------|\n",
                line);

    //fprintf(file,"    |                                           |\n");


    if(bc->numOutputs) {
        uint32_t i=0;
        fprintf(file,
                "    |%s|\n"
                "    |---------------   OUTPUTS   ---------------|\n"
                "    |%s|\n"
                "    |    port_number             bytes          |\n"
                "    |   -------------    -----------------------|\n",
                line, line);
        for(; i<bc->numOutputs; ++i)
            fprintf(file,
                "    |          %4" PRIu32 "      %21" PRIu64 "  |\n",
                    i, bc->outputCounts[i]);
        if(bc->numOutputs == 1) i = 0;
        fprintf(file,
                "    |         total      %21" PRIu64 "  |\n"
                "    |%s|\n",
                bc->outputCounts[i],
                line);
    } else
        fprintf(file,
                "    |%s|\n"
                "    |-------------   NO OUTPUTS   --------------|\n",
                line);

    fprintf(file,
                "    |%s|\n",
                line);

    //fprintf(file,"  ***********************************************\n");
}

void PrintStreamHeader(const struct QsFilter *f) {

    fprintf(stderr,
        "  |*********************************************|\n"
        "  |*****          Stream %3" PRIu32
        "               *****|\n"
        "  |*********************************************|\n"
        "  |                                             |\n",
        qsFilterStreamId(f));
}


static void
CleanFilterByteCounter(const char *pName,
        struct FilterBytesCounter *bc) {

    DASSERT(bc);
    DASSERT(bc->filter);
    DSPEW("Cleaning up filter parameter \"%s:%s\"",
            qsFilterName(bc->filter), pName);

    // Now may be a good time to print a summary:
    PrintSummary(bc, stderr);

    if(bc->numInputs) {
#ifdef DEBUG
        DASSERT(bc->inputCounts);

        // Zeroing the counters may help in debugging.  In general zeroing
        // memory before freeing it can help debugging, so long as we have
        // all the DASSERT() checks in the code.
        memset(bc->inputCounts, 0,
                bc->numInputs*sizeof(*bc->inputCounts));
        if(bc->numInputs > 1)
            // We have a total input counter too.
            memset(bc->inputCounts + bc->numInputs, 0,
                    sizeof(*bc->inputCounts));
#endif
        free(bc->inputCounts);
    }
#ifdef DEBUG
    else
        DASSERT(bc->inputCounts == 0);
#endif


    if(bc->numOutputs) {
#ifdef DEBUG
        DASSERT(bc->outputCounts);

        // Zeroing the counters may help in debugging.
        memset(bc->outputCounts, 0,
                bc->numOutputs*sizeof(*bc->outputCounts));
        if(bc->numOutputs > 1)
            // We have a total output counter too.
            memset(bc->outputCounts + bc->numOutputs, 0,
                    sizeof(*bc->outputCounts));
#endif
        free(bc->outputCounts);
    }
#ifdef DEBUG
    else
        DASSERT(bc->outputCounts == 0);
#endif

#ifdef DEBUG
    memset(bc, 0, sizeof(*bc));
#endif

    free(bc);
}


int construct(int argc, const char **argv) {

    DSPEW();

    printSummary = !qsOptsGetBool(argc, argv, "printNoSummary");


    // TODO: parse options.

    return 0; // success
}


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

    if(numInputs > 1) {
        // zero the total
        bc->inputCounts[numInputs] = 0;
        for(uint32_t i=0; i<numInputs; ++i)
            // Add to the port count and the total is the sum of all.
            bc->inputCounts[numInputs] += (bc->inputCounts[i] +=
                    lenIn[i]);
    } else if(numInputs) {
        // There is just one port to count.
        bc->inputCounts[0] += lenIn[0];
    }

    if(numOutputs > 1) {
        // zero the total
        bc->outputCounts[numOutputs] = 0;
        for(uint32_t i=0; i<numOutputs; ++i)
            // Add to the port count and the total is the sum of all.
            bc->outputCounts[numOutputs] += (bc->outputCounts[i] +=
                    lenOut[i]);
    } else if(numOutputs) {
        // There is just one port to count.
        bc->outputCounts[0] += lenOut[0];
    }

    return 0;
}


static uint32_t printingStreamId = -1;


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

    // Allocate input counters
    if(numInputs == 1) {
        // We only have one counter for both port 0 and the total
        // since the total has the same count as port 0.
        bc->inputCounts = calloc(1, sizeof(*bc->inputCounts));
        ASSERT(bc->inputCounts, "calloc(1,%zu) failed",
                sizeof(*bc->inputCounts));
    } else if(numInputs > 1) {
        // We require 1 more counter for the total.
        bc->inputCounts = calloc(numInputs + 1,
                sizeof(*bc->inputCounts));
        ASSERT(bc->inputCounts, "calloc(%" PRIu32 ",%zu) failed",
                numInputs + 1, sizeof(*bc->inputCounts));
    }

    // Allocate output counters
    if(numOutputs == 1) {
        // We only have one counter for both port 0 and the total
        // since the total has the same count as port 0.
        bc->outputCounts = calloc(1, sizeof(*bc->outputCounts));
        ASSERT(bc->outputCounts, "calloc(1,%zu) failed",
                sizeof(*bc->outputCounts));
    } else if(numOutputs > 1) {
        // We require 1 more counter for the total.
        bc->outputCounts = calloc(numOutputs + 1,
                sizeof(*bc->outputCounts));
        ASSERT(bc->outputCounts, "calloc(%" PRIu32 ",%zu) failed",
                numOutputs + 1, sizeof(*bc->outputCounts));
    }

    // Make a parameter object for each input port and an input total
    if(numInputs) {
        uint32_t i=0;
        for(; i<numInputs; ++i) {
            char pName[16];
            snprintf(pName, 16, "bytesIn%" PRIu32, i);
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

    DSPEW("filter=\"%s\"", qsFilterName(f));

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
            qsParameterDestroyByFilter(f, pName);
        }
        qsParameterDestroyByFilter(f,"bytesInTotal");
    }

    //if(strcmp(qsFilterName(f), "tests/sequenceGen") == 0)
    //    qsDictionaryPrintDot(f->parameters, stderr);


    if(numOutputs) {
        uint32_t i=0;
        for(; i<numOutputs; ++i) {
            char pName[16];
            snprintf(pName, 16, "bytesOut%" PRIu32, i);
            qsParameterDestroyByFilter(f, pName);
        }
        qsParameterDestroyByFilter(f, "bytesOutTotal");
    }

    return 0;
}
