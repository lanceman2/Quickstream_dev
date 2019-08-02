#include <stdlib.h>
#ifndef _GNU_SOURCE
// For the GNU version of basename() that never modifies its argument.
#define _GNU_SOURCE
#endif
#include <string.h>

// The public installed user interfaces:
#include "../include/qsapp.h"


// Private interfaces.
#include "../lib/qsapp.h"
#include "../lib/debug.h"
#include "./list.h"

struct QsApp *qsAppCreate(void) {

    struct QsApp *app = calloc(1, sizeof(*app));

    return app;
}


int qsAppDestroy(struct QsApp *app) {

    DASSERT(app, "");
    free(app);
    return 0; // success
}


struct QsFilter *qsAppFilterLoad(struct QsApp *app,
        const char *fileName, const char *loadName) {

    DASSERT(app, "");
    DASSERT(fileName, "");
    DASSERT(strlen(fileName) > 0, "");

    const char *name = loadName;

    if(!name || !(*name))
        name = loadName;

    struct QsFilter *f = AllocAndAddToFilterList(app, name);

    INFO("Successfully loaded module Filter %s as %s", fileName, f->name);

    return f; // success
}

