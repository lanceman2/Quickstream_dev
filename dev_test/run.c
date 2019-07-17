#include <stdio.h>

#include "../include/fsapp.h"
#include "../lib/debug.h"


int main(void) {

    INFO("hello faststream version %s", FS_VERSION);

    struct FsApp *app = fsAppCreate();

    if(!fsAppFilterLoad(app, "stdin", 0)) {
        fsAppDestroy(app);
        return 1;
    }

    fsAppDestroy(app);
    return 0;
}
