#include <stdlib.h>
#ifndef _GNU_SOURCE
// For the GNU version of basename() that never modifies its argument.
#define _GNU_SOURCE
#endif
#include <string.h>

// The public installed user interfaces:
#include "../include/fsapp.h"


// Private interfaces.
#include "../lib/fsapp.h"
#include "../lib/debug.h"
#include "./list.h"



int fsAppDestroy(struct FsApp *app) {

    DASSERT(app, "");
    free(app);
    return 0; // success
}


struct FsFilter *fsAppFilterLoad(struct FsApp *app,
        const char *fileName, const char *loadName) {

    DASSERT(app, "");
    DASSERT(fileName, "");
    DASSERT(strlen(fileName) > 0, "");

    const char *name = loadName;

    if(!name || !(*name))
        name = loadName;

    struct FsFilter *f = AllocAndAddToFilterList(app, name);

    INFO("Successfully loaded module Filter %s as %s", fileName, f->name);

    return f; // success
}

