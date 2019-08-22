#include <stdio.h>
#include <unistd.h>
#include <signal.h>


#include "../include/qsapp.h"
#include "../lib/debug.h"


static void catcher(int signum) {

    SPEW("Caught signal %d\n", signum);
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
    if(qsStreamConnectFilters(stream, f[numFilters-1], f[1])) {
        qsAppDestroy(app);
        return 1;
    }
    if(qsStreamConnectFilters(stream, f[numFilters-1], f[4])) {
        qsAppDestroy(app);
        return 1;
    }
#endif

    qsAppDisplayFlowImage(app, false);

    if(qsStreamStart(stream, false) >= 0)
        qsStreamStop(stream);


    qsAppDestroy(app);


    WARN("SUCCESS");

    return 0;
}
