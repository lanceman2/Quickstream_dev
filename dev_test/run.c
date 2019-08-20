#include <stdio.h>

#include "../include/qsapp.h"
#include "../lib/debug.h"


int main(void) {

    INFO("hello quickstream version %s", QS_VERSION);

    struct QsApp *app = qsAppCreate();

    if(!qsAppFilterLoad(app, "/foo/stdin.so", 0)) {
        qsAppDestroy(app);
        return 1;
    }

    qsAppDestroy(app);
    return 0;
}
