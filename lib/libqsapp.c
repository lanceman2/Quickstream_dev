#include <stdlib.h>
#ifndef _GNU_SOURCE
// For the GNU version of basename() that never modifies its argument.
#define _GNU_SOURCE
#endif
#include <string.h>
#include <alloca.h>

// The public installed user interfaces:
#include "../include/qsapp.h"


// Private interfaces.
#include "../lib/qsapp.h"
#include "../lib/debug.h"
#include "./list.h"

// Probability
#define DIR_CHAR '/'


struct QsApp *qsAppCreate(void) {

    struct QsApp *app = calloc(1, sizeof(*app));

    return app;
}


int qsAppDestroy(struct QsApp *app) {

    DASSERT(app, "");
    DSPEW();
    free(app);
    return 0; // success
}


struct QsFilter *qsAppFilterLoad(struct QsApp *app,
        const char *fileName, const char *_loadName) {

    DASSERT(app, "");
    DASSERT(fileName, "");
    DASSERT(strlen(fileName) > 0, "");
    DASSERT(strlen(fileName) < 1024*2, "");

    char *loadName = (char *) _loadName;

    if(!loadName || !(*loadName)) {

        size_t len = strlen(fileName) + 1;
        char *s = loadName = alloca(len);
        strncpy(s, fileName, len);
        // loadName = "/the/filename.so"
        // loadName[0] => '/'

        DSPEW("loadName=\"%s\"", loadName);
        if(len > 4) {
            loadName += len - 4;
            // loadName[0] =>  '.'
            if(strcmp(loadName, ".so") == 0) {
                // remove the ".os"
                loadName[0] = '\0';
                --loadName;
                // loadName[0] != '\0'
            }
            // now we are at the end of the name
            //
            while(loadName != s && *loadName != DIR_CHAR)
                loadName--;
            if(*loadName == DIR_CHAR)
                ++loadName;
            // loadName = "filename"
        }
    }

    struct QsFilter *f = AllocAndAddToFilterList(app, loadName);

    INFO("Successfully loaded module Filter %s with name \"%s\"", fileName, f->name);

    return f; // success
}

