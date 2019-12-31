#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#include "../../include/qsapp.h"

// Turn on spew macros for this file.
#ifndef SPEW_LEVEL_DEBUG
#  define SPEW_LEVEL_DEBUG
#endif
#include "../../lib/debug.h"



static void catcher(int signum) {

    WARN("Caught signal %d\n", signum);
    fprintf(stderr, "\n Try:  gdb -pid %u\n\n", getpid());
    while(1) usleep(10000);
}



int main(void) {

    signal(SIGSEGV, catcher);
    setenv("QS_MODULE_PATH", "/home/foo:..:../../lib/quickstream/plugins:..", true);

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

        if(prevF)
            qsStreamConnectFilters(s, prevF, f[i], 0, QS_NEXTPORT);

        prevF = f[i];
        ++i;
    }

    f[i] = qsAppFilterLoad(app, "tests/sleep", 0, 0, 0);
    qsStreamConnectFilters(s, f[0], f[i], 0, QS_NEXTPORT);
    qsStreamConnectFilters(s, f[i], f[2], 0, QS_NEXTPORT);
    qsStreamConnectFilters(s, f[0], f[i], 1, QS_NEXTPORT);
    qsStreamConnectFilters(s, f[i], f[2], 0, QS_NEXTPORT);

    i++;

    f[i] = qsAppFilterLoad(app, "tests/sleep.so", "foodog", 0, 0);
    qsStreamConnectFilters(s, f[0], f[i], 0, QS_NEXTPORT);
    qsStreamConnectFilters(s, f[i], f[2], 0, QS_NEXTPORT);
    qsStreamConnectFilters(s, f[0], f[i], 0, QS_NEXTPORT);
    qsStreamConnectFilters(s, f[i], f[2], 0, QS_NEXTPORT);

    i++;



    qsStreamReady(s);
    qsAppPrintDotToFile(app, QSPrintDebug, stdout);
    qsAppDisplayFlowImage(app, 0, false);
    qsAppDisplayFlowImage(app, QSPrintDebug, false);


    //qsStreamDestroy(s);

    qsAppDestroy(app);


    printf("\n"
            "-- You should see two dot images "
            "with imageMagick, one with detail and one without. --\n"
            "\n");

    return 0;

fail:

    qsAppDestroy(app);
    WARN("FAILURE");
    return 1;
}
