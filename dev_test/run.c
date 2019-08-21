#include <stdio.h>
#include <unistd.h>

#include "../include/qsapp.h"
#include "../lib/qsapp.h"
#include "../lib/debug.h"


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
    qsAppPrintDotToFile(app, stdout);
    qsAppDisplayFlowImage(app, false);
 
stream = qsAppStreamCreate(app);
    if(!stream) {
        qsAppDestroy(app);
        return 1;
    }

// TODO: We need to make this fail.

    if(qsStreamConnectFilters(stream, f0, f1)) {
        qsAppDestroy(app);
        return 1;
    }

    if(qsStreamConnectFilters(stream, f2, f1)) {
        qsAppDestroy(app);
        return 1;
    }

    qsAppPrintDotToFile(app, stdout);
    qsAppDisplayFlowImage(app, false);
    

    qsStreamDestroy(stream);


    qsAppDestroy(app);


    DSPEW("SUCCESS");

    return 0;
}
