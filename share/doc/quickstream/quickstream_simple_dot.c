#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#include "../../../include/quickstream/app.h"

// Turn on spew macros for this file.
#ifndef SPEW_LEVEL_DEBUG
#  define SPEW_LEVEL_DEBUG
#endif
#include "../../../lib/debug.h"



static void catcher(int signum) {

    WARN("Caught signal %d\n", signum);
    fprintf(stderr, "\n Try:  gdb -pid %u\n\n", getpid());
    while(1) usleep(10000);
}



int main(void) {

    signal(SIGSEGV, catcher);
    setenv("QS_MODULE_PATH", "../../../lib/quickstream/plugins:..", true);

    INFO("hello quickstream version %s", QS_VERSION);

    struct QsApp *app = qsAppCreate();
    if(!app)
        return 1;

    struct QsStream *s = qsAppStreamCreate(app);

    struct QsFilter *f0 = qsAppFilterLoad(app, "stdin", 0, 0, 0);
    struct QsFilter *f1 = qsAppFilterLoad(app, "tests/copy", "transmogifer", 0, 0);
    struct QsFilter *f2 = qsAppFilterLoad(app, "stdout", 0, 0, 0);

    if(!f0 || !f1 || !f2) goto fail;

    qsStreamConnectFilters(s, f0, f1, 0, 0);
    qsStreamConnectFilters(s, f1, f2, 0, 0);

    qsStreamReady(s);
    qsAppPrintDotToFile(app, QSPrintDebug, stdout);

    qsAppDestroy(app);

    return 0;

fail:

    qsAppDestroy(app);
    WARN("FAILURE");
    return 1;
}
