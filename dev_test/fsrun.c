#include <stdio.h>

#include "../include/fs.h"
#include "../lib/debug.h"



int main(void) {

    INFO("hello faststream version %s", FS_VERSION);

    struct Fs *fs = fsCreate();

    if(fsLoad(fs, "stdin", 0)) {
        fsDestroy(fs);
        return 1;
    }

    fsDestroy(fs);
    return 0;
}
