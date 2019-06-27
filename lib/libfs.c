#include <stdlib.h>


#include "../include/fs.h"
#include "../lib/debug.h"


static int filterCount = 0;


struct Fs {

    // TODO: make this other than a simple linked list for
    // faster search.  Functions to change:
    // addFilterToList(), 
    //
    // List of filters.
    struct Filter *filters;
};


struct Stream {


};


struct Filter {

    int id; // 

    const char *name; // unique name for a given struct Fs

    int (* constructor)(void);
    int (* destructor)(void);
    int (* start)(int numInChannels, int numOutChannels);
    int (* stop)(int numInChannels, int numOutChannels);
    int (* input)(void *buffer, size_t len, int inputChannelNum);

    struct Filter *next; // 
};


struct Fs *fsCreate(void) {

    struct Fs  *fs = calloc(1, sizeof(*fs));
    ASSERT(fs, "calloc(1,%zu) failed", sizeof(*fs));
    return fs;
}


int fsDestroy(struct Fs *fs) {

    DASSERT(fs, "");

    free(fs);

    return 0; // success
}



static inline void addFilterToList(struct Fs *fs, struct Filter *f) {

    struct Filter *lastF = fs->filters;
    if(!lastF) {
        fs->filters = f;
        return;
    }
    while(lastF->next) lastF = lastF->next;
    lastF->next = f;
    DASSERT(0 == f->next, "");
}



int fsLoad(struct Fs *fs, const char *fileName, const char *loadName) {

    DASSERT(fs, "");
    struct Filter *f = calloc(1, sizeof(*f));
    ASSERT(f, "calloc(1, %zu) failed", sizeof(*f));

    f->id = filterCount++;
    addFilterToList(fs, f);

    INFO("Loaded module Filter %d: %s", f->id, fileName);


    return 0; // success
}





