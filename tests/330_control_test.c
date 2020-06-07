#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>

#include "../lib/debug.h"
#include "../include/quickstream/filter.h"
#include "../include/quickstream/parameter.h"
#include "../include/quickstream/app.h"
#include "../lib/qs.h"
#include "../lib/Dictionary.h"


static
void catcher(int sig) {
    fprintf(stderr, "Caught signal %u\n"
            "\n"
            "  Try:  gdb -pid %u\n\n"
            "Will now sleep ...\n",
            sig, getpid());
    while(1) {
        usleep(10000);
    }
}


double t = 2.32435;



static
int getCallback(
        const void *value, void *stream,
        const char *filterName, const char *pName, 
        enum QsParameterType type, void *userData) {

    fprintf(stderr, "control_test getCallback(value=%lg,"
            " stream=%p, \"%s\","
            "\"%s\", type=%d, userData=%zu)\n",
            *(double *) value, stream, filterName, pName,
            type, (uintptr_t) userData);

    ASSERT(t == *(double *) value);

    return 0;
}



int main(int argc, char **argv) {

    signal(SIGSEGV, catcher);

    struct QsApp *app = qsAppCreate();
    ASSERT(app);
    struct QsStream *s = qsAppStreamCreate(app);
    ASSERT(s);

    struct QsFilter *f = qsStreamFilterLoad(s, "tests/passThrough", "passThrough", 0, 0);

    ASSERT(qsParameterGet(s, "passThrough", "sleep",
                QsDouble, getCallback, 0, 0) == 1);

    ASSERT(qsParameterGet(s, "passThrough", "sleep",
                QsDouble, getCallback, (void *) 1, 0) == 1);

    ASSERT(qsParameterGet(s, "passThrough", "sleep",
                QsDouble, getCallback, (void *) 2, 0) == 1);

    ASSERT(qsParameterSet(s, "passThrough",
                "sleep", QsDouble, &t) == 0);


    qsDictionaryPrintDot(f->parameters, stdout);


    ASSERT(qsAppDestroy(app) == 0);
    
    fprintf(stderr, "\nTry:  %s | display\n\n", argv[0]);


    fprintf(stderr, "SUCCESS\n");
    
    return 0;
}
