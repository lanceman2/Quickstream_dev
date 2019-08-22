#include <stdio.h>
#include <unistd.h>
#include <signal.h>

//#define TEST_PRIVATE


#include "../include/qsapp.h"
#include "../lib/debug.h"

#ifdef TEST_PRIVATE
#  include "../lib/qsapp.h" // private interfaces
#endif


static void catcher(int signum) {

    SPEW("Caught signal %d\n", signum);
    while(1) usleep(10000);
}


int main(void) {

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

    if(qsFilterUnload(f2)) {
        qsAppDestroy(app);
        return 1;
    }

    if(!(f2 = qsAppFilterLoad(app, "stdin.so", 0))) {
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


    if(qsStreamStart(stream, false) >= 0)
        qsStreamStop(stream);


    struct QsThread *t = qsStreamThreadCreate(stream);

    qsThreadAddFilter(t, f1);

    qsThreadDestroy(t);


    qsAppPrintDotToFile(app, stdout); // private to ../lib/ code.
    qsAppDisplayFlowImage(app, false);

    qsStreamRemoveFilter(stream, f2);
    //qsStreamConnectFilters(stream, f2, f1);


    qsAppPrintDotToFile(app, stdout); // private to ../lib/ code.
    qsAppDisplayFlowImage(app, false);

    qsStreamDestroy(stream);

    qsAppPrintDotToFile(app, stdout); // private to ../lib/ code.
    qsAppDisplayFlowImage(app, false);



    qsAppDestroy(app);


    WARN("SUCCESS");

    return 0;
}
