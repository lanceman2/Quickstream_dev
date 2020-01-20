#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#include "../../../include/quickstream/app.h"
#include "../../../lib/debug.h"

static void catcher(int signum) {

    WARN("Caught signal %d\n", signum);
    fprintf(stderr, "\n Try:  gdb -pid %u\n\n", getpid());
    while(1) usleep(10000);
}



int main(void) {

    signal(SIGSEGV, catcher);
    setenv("QS_MODULE_PATH", "../../../lib/quickstream/plugins:..", true);

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

    qsAppDestroy(app);

    return 0;

fail:

    qsAppDestroy(app);
    WARN("FAILURE");
    return 1;
}
