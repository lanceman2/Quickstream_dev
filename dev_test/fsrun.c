#include <stdio.h>

#include "../include/fs.h"
#include "../include/fsf.h"
#include "../lib/debug.h"



int main(void) {

    printf("hello faststream version %s\n", FS_VERSION);

    struct Fs *fs = fsCreate();

    if(fsLoad(fs, "stdin", 0)) {
        fsDestroy(fs);
        return 1;
    }

    fsDestroy(fs);
    return 0;
}
