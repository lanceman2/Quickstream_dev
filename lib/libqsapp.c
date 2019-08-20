#ifndef _GNU_SOURCE
// For the GNU version of basename() that never modifies its argument.
// and For the GNU version of dlmopen()
#  define _GNU_SOURCE
#endif
#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include <stdlib.h>
#include <inttypes.h>
#include <dlfcn.h>
#include <unistd.h>

// The public installed user interfaces:
#include "../include/qsapp.h"


// Private interfaces.
#include "../lib/qsapp.h"
#include "../lib/debug.h"
#include "./list.h"
#include "./GetPath.h"



struct QsApp *qsAppCreate(void) {

    struct QsApp *app = calloc(1, sizeof(*app));

    return app;
}


int qsAppDestroy(struct QsApp *app) {

    DASSERT(app, "");
    DSPEW();

    // First cleanup filters in this app list
    struct QsFilter *f = app->filters;
    while(f) {
        struct QsFilter *nextF = f->next;
        // Destroy this filter f.
        FreeFilter(f);
        f = nextF;
    }

    // Now cleanup this app.
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
        //
        // Derive a filter name from the fileName.
        //
        size_t len = strlen(fileName) + 1;
        char *s = loadName = alloca(len);
        strncpy(s, fileName, len);
        // loadName = "/the/filename.so"
        // loadName[0] => '/'
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
    } else if(FindFilterNamed(app, _loadName)) {
        //
        // Because they requested a particular name we can fail here.
        //
        ERROR("Filter name \"%s\" is in use already", _loadName);
        return 0;
    }

    char *path = 0;

    if(fileName[0] == DIR_CHAR) {

        // We where given full path starting with '/'.
        size_t len = strlen(fileName);

        if(len > 3 && strcmp(&fileName[len-3], ".so") == 0)
            // There is a ".so" suffix.
            path = strdup(fileName);
        else {
            // There is no ".so" suffix.
            path = malloc(len + 4);
            snprintf(path, len + 4, "%s.so", fileName);
        }
    } else
        path = GetPluginPath("filters", fileName);

    //SPEW("Trying path: %s", path);

    //void *handle = dlmopen(LM_ID_NEWLM, path,
    void *handle = dlopen(path,
            RTLD_NOW | RTLD_LOCAL);

    if(!handle) {
        ERROR("Failed to dlmopen(LM_ID_NEWLM,\"%s\",): %s",
                path, dlerror());
        free(path);
        return 0;
    }

    struct QsFilter *f = AllocAndAddToFilterList(app, loadName);
    f->dlhandle = handle;

    // Clear error
    dlerror();

    char *err;

    // If "construct", "destroy", "start", or "stop"
    // are not present, that's okay, they are optional.
    f->construct = dlsym(handle, "construct");
    f->destroy = dlsym(handle, "destroy");
    f->start = dlsym(handle, "start");
    f->stop = dlsym(handle, "stop");

    dlerror(); // clear error
    // "input()" is not optional.
    f->input = dlsym(handle, "input");
    err = dlerror();
    if(err) {
        // We must have a input() function.
        ERROR("dlsym(\"input\") error: %s", err);
        goto cleanup;
    }

    f->input(0, 0, 0);
    if(f->construct) f->construct();

    INFO("Successfully loaded module Filter %s with name \"%s\"",
            path, f->name);
    free(path);

    f->idNum = app->filtersCount++;

    return f; // success

cleanup:

    if(handle) {
        dlerror(); // clear error
        if(dlclose(handle))
            ERROR("dlclose(%p): %s", handle, dlerror());
        // TODO: So what can I do.
    }

    FreeFilterFromList(app, f);
    free(path);
    return 0;
}

