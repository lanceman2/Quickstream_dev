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
"   Usage: tests/monitor\n"
"\n"
" A test controller module that reads and displays Parameters.\n"
"\n"
"\n");
}


int getCallback(
            const void *value,
            void *streamOrApp,
            const char *filterName, const char *pName, 
            enum QsParameterType type, void *userData) {

    if(type == QsDouble)
        printf("%s:%s %lg\n", filterName, pName, *(double *)value);
    else if(type == QsUint64)
        printf("%s:%s %" PRIu64 "\n", filterName, pName, *(uint64_t *)value);

    return 0;
}


int postStart(struct QsStream *s, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs) {

    DSPEW("filter=\"%s\"", qsFilterName(f));

    // TODO: How to remove these getCallbacks before the next start.

    ASSERT(qsParameterGet(s, qsFilterName(f), "^.*$"/*parameter name*/, Any/*any type*/,
           getCallback, 0,  QS_PNAME_REGEX) >= 0);

    return 0;
}
