#include <stdio.h>
#include <unistd.h>

//#define TEST_PRIVATE


#include "../include/qsapp.h"
#include "../lib/debug.h"

#ifdef TEST_PRIVATE
#  include "../lib/qsapp.h" // private interfaces
#endif

int main(void) {

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

    qsAppPrintDotToFile(app, stdout); // private to ../lib/ code.
    qsAppDisplayFlowImage(app, false);

    qsStreamDestroy(stream);
    qsAppDestroy(app);


    WARN("SUCCESS");

    return 0;
}
