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

// TODO: rewrite this code.   Currently it's just a "run test".

    signal(SIGSEGV, catcher);

    INFO("hello quickstream version %s", QS_VERSION);

    struct QsApp *app = qsAppCreate();

    struct QsFilter *f0, *f1, *f2;

    if(!(f0 = qsAppFilterLoad(app, "stdin.so", 0))) {
        qsAppDestroy(app);
        return 1;
    }

    if(!(f2 = qsAppFilterLoad(app, "stdin.so", 0))) {
        qsAppDestroy(app);
        return 1;
    }

    f1 = qsAppFilterLoad(app, "stdout.so", "stdout");

    if(!f1) {
        qsAppDestroy(app);
        return 1;
    }


    struct QsStream *stream = qsAppStreamCreate(app);
    if(!stream) {
        qsAppDestroy(app);
        return 1;
    }

    if(qsStreamConnectFilters(stream, f0, f1)) {
        qsAppDestroy(app);
        return 1;
    }

    if(qsStreamConnectFilters(stream, f2, f1)) {
        qsAppDestroy(app);
        return 1;
    }

    if(qsStreamConnectFilters(stream, f2, f1)) {
        qsAppDestroy(app);
        return 1;
    }

    if(qsStreamConnectFilters(stream, f1, f2)) {
        qsAppDestroy(app);
        return 1;
    }


    qsAppDisplayFlowImage(app, false);

    qsStreamDestroy(stream);

    qsAppDestroy(app);


    WARN("SUCCESS");

    return 0;
}
