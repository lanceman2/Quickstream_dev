#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "../include/qsapp.h"


// Turn on spew macros for this file.
#ifndef SPEW_LEVEL_DEBUG
#  define SPEW_LEVEL_DEBUG
#endif
#include "../lib/debug.h"


static void catcher(int signum) {

    WARN("Caught signal %d\n", signum);
    while(1) usleep(10000);
}


int main(void) {

    signal(SIGSEGV, catcher);

    INFO("hello quickstream version %s", QS_VERSION);

    struct QsApp *app = qsAppCreate();

    const int numFilters = 10;

    struct QsFilter *f[numFilters];

    for(int i=0; i<numFilters; ++i)
        if(!(f[i] = qsAppFilterLoad(app, "stdin.so", 0))) {
            qsAppDestroy(app);
            return 1;
        }

    struct QsStream *stream = qsAppStreamCreate(app);
    if(!stream) {
        qsAppDestroy(app);
        return 1;
    }


    for(int i=0; i<numFilters-1; ++i)
        if(qsStreamConnectFilters(stream, f[i], f[i+1])) {
            qsAppDestroy(app);
            return 1;
        }
#if 0

    qsFilterUnload(f[0]);

    qsAppDisplayFlowImage(app, false);

    f[0] = qsAppFilterLoad(app, "stdin", 0);

    qsStreamConnectFilters(stream, f[0], f[2]);
    qsStreamConnectFilters(stream, f[2], f[0]);

#endif

    qsAppDisplayFlowImage(app, false);


    qsStreamStart(stream);


    qsAppDestroy(app);


    WARN("SUCCESS");

    return 0;
}
