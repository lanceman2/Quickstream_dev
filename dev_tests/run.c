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
    if(!app)
        return 1;

    const char *fn[] = { "stdin.so", "tests/sleep.so", "stdout.so", 0 };
    
    struct QsFilter *prevF = 0;
    struct QsStream *s = qsAppStreamCreate(app);
    if(!s) goto fail;

    for(const char **n=fn; *n; ++n) {
        struct QsFilter *f = qsAppFilterLoad(app, *n, 0, 0, 0);
        if(!f) goto fail;

        if(prevF && qsStreamConnectFilters(s, prevF, f))
            goto fail;

        prevF = f;
    }

    qsStreamPrestart(s);
    //qsAppDisplayFlowImage(app, 0, false);
    qsStreamStart(s);

    qsAppDestroy(app);

    WARN("SUCCESS");
    return 0;

fail:

    qsAppDestroy(app);
    WARN("FAILURE");
    return 1;
}
