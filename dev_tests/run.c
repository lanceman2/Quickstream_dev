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

    struct QsFilter *f0, *f1, *f2;

    if(!(f0 = qsAppFilterLoad(app, "tests/sleep.so", 0, 0, 0))) {
        qsAppDestroy(app);
        return 1;
    }

    if(!(f2 = qsAppFilterLoad(app, "stdin.so", 0, 0, 0))) {
        qsAppDestroy(app);
        return 1;
    }

    f1 = qsAppFilterLoad(app, "tests/sleep.so", "SLEEP", 0, 0);

    if(!f1) {
        qsAppDestroy(app);
        return 1;
    }

    if(qsFilterUnload(f2)) {
        qsAppDestroy(app);
        return 1;
    }

    if(!(f2 = qsAppFilterLoad(app, "tests/sleep", 0, 0, 0))) {
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

#if 0
    if(qsStreamStart(stream, false) >= 0)
        qsStreamStop(stream);
#endif

    struct QsThread *t = qsStreamThreadCreate(stream);

    qsThreadAddFilter(t, f1);

    qsThreadDestroy(t);


    qsAppDisplayFlowImage(app, false);

    qsStreamRemoveFilter(stream, f2);
    //qsStreamConnectFilters(stream, f2, f1);


    qsAppDisplayFlowImage(app, false);

    qsStreamDestroy(stream);

    qsAppDisplayFlowImage(app, false);



    qsAppDestroy(app);


    WARN("SUCCESS");

    return 0;
}
