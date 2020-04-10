#include <string.h>
#include <alloca.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdatomic.h>
#include <pthread.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// The public installed user interfaces:
#include "../include/quickstream/app.h"

// Private interfaces.
#include "debug.h"
#include "Dictionary.h"
#include "qs.h"
#include "GetPath.h"
#include "filterList.h"
#include "LoadDSOFromTmpFile.h"


// Used to find which Controller module calling functions in the
// controller API.
pthread_key_t controllerKey;
static pthread_once_t keyOnce = PTHREAD_ONCE_INIT;

static void MakeKey(void) {
    CHECK(pthread_key_create(&controllerKey, 0));
}




// This cleans up all the Controller resources except it does not remove
// the app dictionary entry, which is done in qsControllerUnload() and the
// qsAppDestroy(); by removing the dictionary entry and letting
// qsDictionarySetFreeValueOnDestroy() do its' thing.
static void
DictionaryDestroyController(struct QsController *c) {

    DASSERT(c);
    DASSERT(c->app);
    DASSERT(c->name);

    if(c->dlhandle) {

        // if c->dlhandle was not set than the construct() returned
        // an error and we do not need to call destroy().

        int (*destroy)(void) = dlsym(c->dlhandle, "destroy");
        if(destroy) {

            DSPEW("Calling controller \"%s\" destroy()", c->name); 
            // This needs to be re-entrant code.
            void *oldController = pthread_getspecific(controllerKey);
            CHECK(pthread_setspecific(controllerKey, c));

            // A controller cannot load itself.
            DASSERT(oldController != c);
            // We use this, mark, flag as a marker that we are in the
            // construct() phase.
            c->mark = _QS_IN_CDESTROY;

            int ret = destroy();

            // Controller, c, is not in destroy() phase anymore.
            c->mark = 0;

            // This will set the thread specific data to 0 for the case
            // when it was 0 before this block of code.
            CHECK(pthread_setspecific(controllerKey, oldController));

            if(ret)
                WARN("Calling controller \"%s\" destroy() returned %d",
                        c->name, ret);
        }

        dlerror(); // clear error
        if(dlclose(c->dlhandle))
            WARN("dlclose(%p): %s", c->dlhandle, dlerror());

        DSPEW("Unloaded and cleaned up controller named \"%s\"",
                c->name);
    }

    free(c->name);

#ifdef DEBUG
    memset(c, 0, sizeof(*c));
#endif

    free(c);
}


static
int FindControllerCallback(const char *key, void *value,
            void *dlhandle) {

    struct QsController *c = value;

    if(c->dlhandle == dlhandle) {
        dlclose(dlhandle);
        c->dlhandle = 0;
        return 1; // found it.
    }

    return 0;
}


static inline bool
FindControllerHandle(struct QsApp *app, struct QsController *c) {
    DASSERT(app);
    DASSERT(c);
    DASSERT(c->dlhandle);

    qsDictionaryForEach(app->controllers, FindControllerCallback,
            c->dlhandle);

    if(c->dlhandle == 0)
        // The same controller module was loaded already.
        return true; // it was found.

    return false;
}


int qsControllerPrintHelp(const char *fileName, FILE *f) {

    if(f == 0) f = stderr;

    char *path = GetPluginPath("controllers/", fileName);

    void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);

    if(!handle) {
        ERROR("Failed to dlopen(\"%s\",): %s", path, dlerror());
        free(path);
        return 1; // error
    }

    dlerror(); // clear error
    void (*help)(FILE *) = dlsym(handle, "help");
    char *err = dlerror();
    if(err) {
        // We must have a input() function.
        ERROR("Module at path=\"%s\" no help() provided:"
                " dlsym(\"help\") error: %s", path, err);
        free(path);
        dlclose(handle);
        return 1; // error
    }

    fprintf(f, "\ncontroller path=%s\n\n", path);
    help(f);

    dlclose(handle);
    free(path);
    return 0; // success
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

    if(loadName && *loadName) {
        if(qsDictionaryFind(app->controllers, loadName)) {
            ERROR("Controller named \"%s\" is in use already", loadName);
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
            // "/controllers/" we add it to the name.
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

    int i;
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

    char *path = GetPluginPath("controllers/", fileName);


    void *dlhandle = dlopen(path, RTLD_NOW | RTLD_LOCAL);

    if(!dlhandle) {
        ERROR("Failed to dlopen(\"%s\",): %s", path, dlerror());
        free(path);
        goto cleanup;
    }
    c->dlhandle = dlhandle;

    if(FindControllerHandle(app, c)) {
        //
        // This DSO (dynamic shared object) file is already loaded.  So we
        // must copy the DSO file to a temp file and load that.  Otherwise
        // we will have just the one plugin loaded, but referred to by two
        // (or more) controllers, which is not what we want.  The temp file
        // will be automatically removed when the process exits.
        //
        c->dlhandle = dlhandle = LoadDSOFromTmpFile(path);
        if(!dlhandle) {
            free(path);
            goto cleanup;
        }
    }

    ASSERT(qsDictionaryInsert(app->controllers, c->name, c, &d) == 0);
    DASSERT(d);

    qsDictionarySetFreeValueOnDestroy(d,
            (void (*)(void *)) DictionaryDestroyController);


    // If these functions are not present, that's okay, they are
    // optional.
    int (* construct)(int argc, const char **argv) =
        dlsym(dlhandle, "construct");
    c->preStart = dlsym(dlhandle, "preStart");
    c->postStart = dlsym(dlhandle, "postStart");
    c->preStop = dlsym(dlhandle, "preStop");
    c->postStop = dlsym(dlhandle, "postStop");

    dlerror(); // clear error
    dlsym(dlhandle, "help");
    char *err = dlerror();
    if(err) {
#ifdef QS_FILTER_REQUIRE_HELP
        // "help()" is not optional.
        // We must have a help() function.
        ERROR("no help() provided: dlsym(\"help\") error: %s", err);
        goto cleanup;
#else
        // help() is optional.
        WARN("Controller \"%s\" module from file=\"%s\" does"
                " not provide a help() function.",
                c->name, path);
#endif
    }

    // We need thread specific data to tell what controller this is when
    // the controller API is called.
    CHECK(pthread_once(&keyOnce, MakeKey));

    DSPEW("construct=%p", construct); 


    if(construct) {

        // This needs to be re-entrant code.
        void *oldController = pthread_getspecific(controllerKey);
        CHECK(pthread_setspecific(controllerKey, c));

        // A controller cannot load itself.
        DASSERT(oldController != c);
        // We use this, mark, flag as a marker that we are in the
        // construct() phase.
        c->mark = _QS_IN_CCONSTRUCT;

        int ret = construct(argc, argv);

        // Controller, c, is not in construct() phase anymore.
        c->mark = 0;

        // This will set the thread specific data to 0 for the case when
        // it was 0 before this block of code.
        CHECK(pthread_setspecific(controllerKey, oldController));

        if(ret) {
            // construct() returned an error.
            if(ret < 0)
                ERROR("In loading Controller \"%s\" from path %s"
                        ": contruct() returned %d",
                        c->name, path, ret);
            free(path);
            // We were all done except this dammed contruct(), so we can
            // call the regular user API cleanup function now, but to stop
            // it from calling the destruct() we close the dlhandle:
            dlclose(c->dlhandle);
            c->dlhandle = 0;
            qsControllerUnload(c);
            return 0; // failure returns Null pointer.
        }
    }

    INFO("Successfully loaded module Controller %s with name \"%s\"",
            path, c->name);

    free(path);

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

    if(!c) {
        // A controller may unload itself and it identifies itself by
        // c = 0.
        c = pthread_getspecific(controllerKey);
        DASSERT(c);
        if(!c) {
            ERROR("qsControllerUnload() No controller argument set");
            return 1;
        }
        DASSERT(c->mark != _QS_IN_CDESTROY);
        if(c->mark == _QS_IN_CDESTROY) return 1;

        DASSERT(c->mark == _QS_IN_CCONSTRUCT ||
                c->mark == _QS_IN_PRESTART ||
                c->mark == _QS_IN_POSTSTART ||
                c->mark == _QS_IN_PRESTOP ||
                c->mark == _QS_IN_POSTSTOP);
    }

    DASSERT(c->app);
    DSPEW("Unloading controller \"%s\"", c->name);

    if(qsDictionaryRemove(c->app->controllers, c->name)) {
        ERROR("Controller \"%s\" not found in app", c->name);
        return -1; // error
    }

    return 0; // success
}
