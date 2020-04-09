#include <string.h>
#include <alloca.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdatomic.h>
#include <pthread.h>


// The public installed user interfaces:
#include "../include/quickstream/app.h"

// Private interfaces.
#include "debug.h"
#include "Dictionary.h"
#include "qs.h"
#include "GetPath.h"

// This cleans up all the Controller resources except it does not remove
// the app dictionary entry, which is done in qsControllerUnload() and the
// qsAppDestroy(); by remove the dictionary entry and letting
// qsDictionarySetFreeValueOnDestroy() do its' thing.
static void
DictionaryDestroyController(struct QsController *c) {

    DASSERT(c);
    DASSERT(c->app);


}


struct QsController *qsAppControllerLoad(struct QsApp *app,
        const char *fileName,
        const char *loadName,
        int argc, const char **argv) {

    DASSERT(app);
    DASSERT(fileName);
    DASSERT(*fileName);

    struct QsDictionary *d;
    struct QsController *c = calloc(1, sizeof(*c));
    ASSERT(c, "calloc(1,%zu) failed", sizeof(*c));
    c->app = app;

    char *name = 0;
    char *name_mem = 0;
    int i;

    if(loadName && *loadName) {
        if(qsDictionaryFind(app->controllers, loadName)) {
            ERROR("Controller name \"%s\" is in use already", loadName);
            goto cleanup;
        }
        name = (char *) loadName;
    } else {
        //
        // Derive a controller name from the fileName.
         //
        size_t len = strlen(fileName) + 1;
        char *st = name = alloca(len);
        strncpy(st, fileName, len);
        // name = "foo/controller/test/filename.so"
        // name[0] => '/'
        if(len > 4) {
            name += len - 4;
            // name[0] =>  '.'
            if(strcmp(name, ".so") == 0) {
                // remove the ".os"
                name[0] = '\0';
                --name;
                // name[0] != '\0'
            }
            // now we are at the end of the name
            //
            while(name != st && *name != DIR_CHAR)
                name--;
            // name = "/filename"

            // If the next directory in the path is not named
            // "/filter/" we add it to the name.
            const size_t flen = strlen(DIR_STR "controllers");
            while(!(name - st > flen &&
                    strncmp(name-flen, DIR_STR "controllers", flen)
                    == 0) &&
                    name > st + 2) {
                --name;
                while(name != st && *name != DIR_CHAR)
                    name--;
                // name = "/test/filename"
            }
            if(*name == DIR_CHAR)
                ++name;
        }
    }

    if(strlen(name) > _QS_FILTER_MAXNAMELEN) {
        ERROR("Controller name \"%s\" is too long", name);
        goto cleanup;
    }

    name_mem = malloc(strlen(name) + 9);
    ASSERT(name_mem, "malloc() failed");
    strcpy(name_mem, name);

#define TRYS      10002

    for(i=2; i<10002; ++i) {
        if(qsDictionaryFind(app->controllers, name_mem) == 0)
            break;
        sprintf(name_mem, "%s-%d", name, i);
    }

    if(i == TRYS) {
        // This should not happen.
        ERROR("Controller name generation failed");
        goto cleanup;
    }

    c->name = name_mem;

    ASSERT(qsDictionaryInsert(app->controllers, c->name, c, &d) == 0);
    DASSERT(d);

    qsDictionarySetFreeValueOnDestroy(d,
            (void (*)(void *)) DictionaryDestroyController);

    return c; // success

cleanup:

    if(c->name)
        free(c->name);
    free(c);
    return 0; // failure
}



/*
 * return 0 on success or non-zero on failure.
 */
int qsControllerUnload(struct QsController *c) {

    DASSERT(c);
    DASSERT(c->app);
    DSPEW("Unloading controller \"%s\"", c->name);

    if(qsDictionaryRemove(c->app->controllers, c->name)) {
        ERROR("Controller \"%s\" not found in app", c->name);
        return -1; // error
    }

    return 0; // success
}

/*
 * \return 0 is help() is found and called.
 */
int qsControllerPrintHelp(const char *controllerName, FILE *f) {

    return 0; // success
}
