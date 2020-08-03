
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
" A test controller module that reads and prints parameters.\n"
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

    ASSERT(qsParameterGet(s, qsFilterName(f),
                "^.*$"/*parameter name egex*/,
                Any/*any type*/,
                getCallback, 0, 0,
                QS_PNAME_REGEX | QS_KEEP_AT_RESTART | QS_KEEP_ONE
                ) >= 0);

    return 0;
}
