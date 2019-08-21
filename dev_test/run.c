#include <stdio.h>

#include "../include/qsapp.h"
#include "../lib/debug.h"


int main(void) {

    INFO("hello quickstream version %s", QS_VERSION);

    struct QsApp *app = qsAppCreate();

    if(!qsAppFilterLoad(app, "stdin.so", 0)) {
        qsAppDestroy(app);
        return 1;
    }

    if(!qsAppFilterLoad(app, "stdin.so", 0)) {
        qsAppDestroy(app);
        return 1;
    }

    if(!qsAppFilterLoad(app, "stdout.so", "stdout")) {
        qsAppDestroy(app);
        return 1;
    }

    qsAppDestroy(app);


    DSPEW("SUCCESS");

    return 0;
}
