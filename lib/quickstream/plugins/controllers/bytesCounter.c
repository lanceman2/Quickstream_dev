// This controller module is part of a test in tests/362_controller_dummy
//
// If you change this file you'll likely brake that test.  If you need to
// edit this, make sure that that you also fix tests/362_controller_dummy,
// without changing the nature of that test.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../../include/quickstream/app.h"
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
//
#include "../../../../lib/Dictionary.h"



void help(FILE *f) {
    fprintf(f,
"   Usage: bytesCounter\n"
"\n"
" A controller module that adds a bytes counter for\n"
" all filters in all streams.\n"
"\n"
"\n                OPTIONS\n"
"\n"
"     --filter NAME0 [...]   add the bytes counter for just filters\n"
"                           with the following names NAME0 ...\n"
"                           Only the last --filter option will be used.\n"
"\n"
"\n");
}



struct ByteCounter {

    uint64_t count;

    // singly linked list of counters
    // Starting at total -> 0 -> 1 -> 2 -> NULL
    //
    struct ByteCounter *next;

    struct QsFilter *filter;

    uint32_t port; // -1 for total

    bool isInput;
};


static void
CleanUpByteCounter(const char *pName, struct ByteCounter *bc) {

    DASSERT(bc);
    DSPEW("Cleaning up filter parameter \"%s:%s\"",
            qsFilterName(bc->filter), pName);

#ifdef DEBUG
    memset(bc, 0, sizeof(*bc));
#endif

    free(bc);
}


static
int setByteCountCB(void *value, const char *pName,
            struct ByteCounter *bc) {

    if(bc->port != -1) {
        // Not a total.  Just one port.
        bc->count += *((uint64_t *) value);
        return 0;
    }

    // This needs to get a total by adding all the ports.
    if(bc->next) {
        bc->count = 0;
        for(struct ByteCounter *b = bc->next; b; b = b->next)
            bc->count += b->count;
        return 0;
    }

    // There is one port and only the total count.
    bc->count += *((uint64_t *) value);

    return 0;
}


static struct ByteCounter *
AddFilterByteCounter( struct QsFilter *f,  uint32_t port,
        const char *pName, bool isInput) {

    struct ByteCounter *bc = malloc(sizeof(*bc));
    ASSERT(bc, "malloc(%zu) failed", sizeof(*bc));

    bc->count = 0;
    bc->filter = f;
    bc->port = port;
    bc->isInput = isInput;

    qsParameterCreateForFilter(f, pName, QsUint64, 
            (int (*)(void *value, const char *pName,
                void *userData)) setByteCountCB,
            (void (*)(const char *pName, void *userData))
                CleanUpByteCounter,
            bc/*userData*/);

    return bc;
}


int construct(int argc, const char **argv) {


    DSPEW();

    return 0; // success
}


int preStart(struct QsStream *s, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs) {

    DSPEW("filter=\"%s\"", qsFilterName(f));

    // A counter for all inputs.
    struct ByteCounter *bc;
    bc = AddFilterByteCounter(f, -1, "totalBytesIn", true);

    if(numInputs > 1)
        for(uint32_t i=0; i<numInputs; ++i) {
            char pName[16];
            snprintf(pName, 16, "bytesIn%" PRIu32, i);
            bc->next = AddFilterByteCounter(f, i, pName, true);
            bc = bc->next;
        }
    bc->next = 0; // terminate the list.


    // A counter for all outputs.
    bc = AddFilterByteCounter(f, -1, "totalBytesOut", false);

    if(numOutputs > 1)
        for(uint32_t i=0; i<numOutputs; ++i) {
            char pName[16];
            snprintf(pName, 16, "bytesOut%" PRIu32, i);
            bc->next = AddFilterByteCounter(f, i, pName, false);
            bc = bc->next;
        }
    bc->next = 0; // terminate the list.

    return 0; // keep calling for all filters.
}
