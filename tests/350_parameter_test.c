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

    WARN("getCallback(value=%lg,"
            " stream=%p, \"%s\","
            "\"%s\", type=%" PRIu32 ", userData=%zu)\n",
            *(double *) value, stream, filterName, pName,
            type, (uintptr_t) userData);

    ASSERT(t == *(double *) value);

    return 0;
}


static
int findParameter(struct QsStream *stream,
        const char *filterName, const char *pName,
        enum QsParameterType type, void *userData) {

    WARN("stream-%" PRIu32 " (type=%d) parameter \"%s:%s\"",
            stream->id, type, filterName, pName);
    return 0;
}



int main(int argc, char **argv) {

    signal(SIGSEGV, catcher);

    struct QsApp *app = qsAppCreate();
    ASSERT(app);
    struct QsStream *s = qsAppStreamCreate(app);
    ASSERT(s);

    struct QsFilter *f = qsStreamFilterLoad(s, "tests/parameter", 0, 0, 0);
    qsStreamFilterLoad(s, "tests/parameter", 0, 0, 0);
    qsStreamFilterLoad(s, "tests/parameter", 0, 0, 0);

    ASSERT(qsParameterGet(s, "tests/parameter", "par 0",
                QsDouble, getCallback, 0, 0, 0) == 1);

    ASSERT(qsParameterGet(s, "tests/parameter", "par 1",
                QsDouble, getCallback, 0, (void *) 1, 0) == 1);

    qsParameterForEach(app, s, 0, 0, 0, findParameter,
            0/*userData*/, 0/*flags*/);

    ASSERT(qsParameterSet(s, "tests/parameter",
                "par 0", QsDouble, &t) == 0);

    t = 3434.8913;
    ASSERT(qsParameterSet(s, "tests/parameter",
                "par 0", QsDouble, &t) == 0);



    qsDictionaryPrintDot(f->parameters, stdout);


    ASSERT(qsAppDestroy(app) == 0);
    
    fprintf(stderr, "\nTry:  %s | display\n\n", argv[0]);


    fprintf(stderr, "SUCCESS\n");
    
    return 0;
}
