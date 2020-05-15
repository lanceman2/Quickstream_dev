// This controller module is part of a test in tests/362_controller_dummy
//
// If you change this file you'll likely brake that test.  If you need to
// edit this, make sure that that you also fix tests/362_controller_dummy,
// without changing the nature of that test.


#include <stdio.h>

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
"   Usage: bytesCount\n"
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


#if 0
static
int setByteCountCB(void *value, const char *pName,
            void *userData) {


    return 0;
}


static int AddFilter(struct QsStream *s, struct QsFilter *f,
        void *userData) {

    
    char *pName = "bytes";

    qsParameterCreateForFilter(f, pName, QsUint64, 
            (int (*)(void *value, const char *pName,
                void *userData)) setByteCountCB, 0/*userData*/);

    return 0; // call with next filter.
}
#endif


int construct(int argc, const char **argv) {

    DSPEW();

    return 0; // success
}


int preStart(struct QsStream *s, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs) {

    DSPEW("filter=\"%s\"", qsFilterName(f));

    return 0; // keep calling for all filters.
}
