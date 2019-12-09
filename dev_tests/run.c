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
    struct QsFilter *f[10];
    struct QsFilter *prevF = 0;
    struct QsStream *s = qsAppStreamCreate(app);
    if(!s) goto fail;
    int i=0;

    for(const char **n=fn; *n; ++n) {
        f[i] = qsAppFilterLoad(app, *n, 0, 0, 0);
        if(!f[i]) goto fail;

        if(prevF && qsStreamConnectFilters(s, prevF, f[i]))
            goto fail;

        prevF = f[i];
        ++i;
    }

    f[i] = qsAppFilterLoad(app, "tests/sleep", 0, 0, 0);
    if(qsStreamConnectFilters(s, f[0], f[i]))
        goto fail;
    if(qsStreamConnectFilters(s, f[i], f[2]))
        goto fail;


    if(qsStreamConnectFilters(s, f[0], f[i]))
        goto fail;
    if(qsStreamConnectFilters(s, f[i], f[2]))
        goto fail;


    i++;

    qsStreamReady(s);
    
    qsAppPrintDotToFile(app, QSPrintDebug, stdout);
    qsAppDisplayFlowImage(app, 0, false);
    qsAppDisplayFlowImage(app, QSPrintDebug, false);


    qsAppDestroy(app);

    WARN("SUCCESS");
    return 0;

fail:

    qsAppDestroy(app);
    WARN("FAILURE");
    return 1;
}
