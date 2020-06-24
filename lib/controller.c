#include <string.h>
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
#include "LoadDSOFromTmpFile.h"


// Used to find which Controller module calling functions in the
// controller API.
pthread_key_t _qsControllerKey;
static pthread_once_t keyOnce = PTHREAD_ONCE_INIT;

static void MakeKey(void) {
    CHECK(pthread_key_create(&_qsControllerKey, 0));
}


static
void FreeScriptLoaderOnDestroy(struct QsScriptControllerLoader *loader) {

    DASSERT(loader);

    if(loader->dlhandle) {
        // Looks like we loaded one or more python controllers.
        void (*cleanup)(void) = dlsym(loader->dlhandle, "cleanup");
        char *err = dlerror();
        if(err)
            ERROR("no pyControllerLoader cleanup() provided:"
                    " dlsym(,\"cleanup\") error: %s", err);
        else
            cleanup();
        WARN();
        dlclose(loader->dlhandle);
        WARN();
    }
    WARN();
#ifdef DEBUG
    memset(loader, 0, sizeof(*loader));
#endif
    free(loader);
    WARN();
}



// Returns the scriptControllerLoader if it is loaded now, or if it is
// already successfully loaded from before, else returns 0.
//
static inline
struct QsScriptControllerLoader *
CheckLoadScriptLoader(struct QsApp *app, const char *loaderModuleName) {

    struct QsScriptControllerLoader *loader =
        qsDictionaryFind(app->scriptControllerLoaders, loaderModuleName);
    if(loader) return loader;

    char *scriptLoaderPath = GetPluginPath(MOD_PREFIX, "run/",
            loaderModuleName, ".so");
    DASSERT(scriptLoaderPath);

    void *dlhandle = dlopen(scriptLoaderPath, RTLD_NOW | RTLD_LOCAL);
    if(!dlhandle) {
        ERROR("Failed to dlopen(\"%s\",): %s", scriptLoaderPath, dlerror());
        goto fail;
    }
    dlerror(); // clear error
    int (*initialize)(void) = dlsym(dlhandle, "initialize");
    char *err = dlerror();
    if(err) {
        ERROR("Module at path=\"%s\" no initialize() provided:"
                " dlsym(,\"help\") error: %s", scriptLoaderPath, err);
        goto fail;
    }
    // Call the modules first function.
    if(initialize()) {
        ERROR("%s initialize() failed", scriptLoaderPath);
        goto fail;
    }
    dlerror(); // clear error
    void *(*loadControllerScript)(const char *path, int argc,
            const char **argv) = dlsym(dlhandle, "loadControllerScript");
    err = dlerror();
    if(err) {
        ERROR("Module at path=\"%s\" no loadControllerScript() provided:"
                " dlsym(,\"loadControllerScript\") error: %s",
                scriptLoaderPath, err);
        goto fail;
    }

    WARN("loaded module %s", scriptLoaderPath);
 
    free(scriptLoaderPath);

    loader = malloc(sizeof(*loader));
    ASSERT(loader, "malloc(%zu) failed", sizeof(*loader));

    loader->dlhandle = dlhandle;
    loader->loadControllerScript = loadControllerScript;
    struct QsDictionary *d;
    qsDictionaryInsert(app->scriptControllerLoaders, loaderModuleName,
                loader, &d);
    qsDictionarySetFreeValueOnDestroy(d,
            (void (*)(void *)) FreeScriptLoaderOnDestroy);

    return loader; // success

fail:
    if(dlhandle)
        dlclose(dlhandle);
    free(scriptLoaderPath);
    return 0; // error
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
    DASSERT(c->parameters);
 
    qsDictionaryDestroy(c->parameters);


    if(c->dlhandle) {

        // if c->dlhandle was not set than the construct() returned
        // an error and we do not need to call destroy().

        int (*destroy)(void) = dlsym(c->dlhandle, "destroy");
        if(destroy) {

            DSPEW("Calling controller \"%s\" destroy()", c->name); 
            // This needs to be re-entrant code.
            void *oldController = pthread_getspecific(_qsControllerKey);
            CHECK(pthread_setspecific(_qsControllerKey, c));

            // A controller cannot unload itself by calling this.
            DASSERT(oldController != c);
            // We use this, mark, flag as a marker that we are in the
            // construct() phase.
            c->mark = _QS_IN_CDESTROY;

            int ret = destroy();

            // Controller, c, is not in destroy() phase anymore.
            c->mark = 0;

            // This will set the thread specific data to 0 for the case
            // when it was 0 before this block of code.
            CHECK(pthread_setspecific(_qsControllerKey, oldController));

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

    // Remove this controller from the Apps doubly linked list
    //
    if(c->prev) {
        DASSERT(c != c->app->first);
        c->prev->next = c->next;
    } else {
        // c->prev == 0
        DASSERT(c == c->app->first);
        c->app->first = c->next;
    }
    
    if(c->next) {
        DASSERT(c != c->app->last);
        c->next->prev = c->prev;
    } else {
        // c->next == 0
        DASSERT(c == c->app->last);
        c->app->last = c->prev;
    }

    free(c->name);

#ifdef DEBUG
    memset(c, 0, sizeof(*c));
#endif

    free(c);
}


static
int FindControllerCallback(const char *key, struct QsController *c,
            void **dlhandle) {

    if(c->dlhandle == *dlhandle) {
        // Not unique.  Close it.
        dlclose(*dlhandle);
        *dlhandle = 0;
        return 1; // found it.
    }
    return 0;
}

// Make sure the dlhandle is not in the list of controllers in app.
void *
GetUniqueControllerHandle(struct QsApp *app,  void **dlhandle,
        const char *path) {
    DASSERT(app);
    DASSERT(dlhandle);
    DASSERT(*dlhandle);

    qsDictionaryForEach(app->controllers,
            (int (*)(const char *, void *, void *))
                FindControllerCallback, dlhandle);

    if(*dlhandle == 0)
        // Try to load it from a copy of the DSO file.
        return LoadDSOFromTmpFile(path);

    // It's was unique to begin with.
    return *dlhandle;
}


int qsControllerPrintHelp(const char *fileName, FILE *f) {

    if(f == 0) f = stderr;

    char *path = GetPluginPath(MOD_PREFIX, "controllers/",
            fileName, ".so");

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
    char *path = 0;

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
    for(i=2; i<TRYS; ++i) {
        if(qsDictionaryFind(app->controllers, name_mem) == 0)
            break;
        sprintf(name_mem, "%s-%d", name, i);
    }
    c->name = name_mem;

    if(i == TRYS) {
        // This should not happen.
        ERROR("Controller name generation failed");
        goto cleanup;
    }

    path = GetPluginPath(MOD_PREFIX, "controllers/", fileName, ".so");
    DASSERT(path);

    void *dlhandle = dlopen(path, RTLD_NOW | RTLD_LOCAL);

    // The way we tell if this dlhandle is to a file that we already
    // loaded is if we got the same dlhandle returned before and it's
    // in the app list already.

    if(dlhandle && !(dlhandle = GetUniqueControllerHandle(app, &dlhandle,
                    path)))
        // Could not get a unique dlhandle.
        goto cleanup; // fail


    if(!dlhandle) {

        // Okay the loading a DSO failed, so lets see if this is a
        // scripting language file in this path.

        // Load controllers from scripting a language.
        //
        // TODO: add other languages, other than python.
        // Like LUA.
        //

        // CASE PYTHON:
        //
        struct QsScriptControllerLoader *loader = 
            CheckLoadScriptLoader(app, "pythonControllerLoader");
        if(!loader)
            goto cleanup;

        // This is not a loadable DSO (dynamic share object) plugin.
        // And so, it's not a regular compiled DSO from C or C++.
        free(path); // We'll reuse path as the python module file.

        // Python 3.8.2 BUG: Py_Initialize() and Py_InitializeEx() require
        // the PYTHONPATH environment variable to be set or we cannot run
        // any python script from C.
        path = strdup(fileName);
        size_t len = strlen(path);
        if(len > 3 && path[len-1] == 'y' && path[len-2] == 'p' &&
                path[len-3] == '.')
            // Python does not like the .py suffix in the python
            // module filename.
            path[len-3] = '\0';

        // If this is a usable thing:
        // This may be a python module.  That's what we define, at this
        // point, as the only none failure option.
        // If that's the case, we need to consider looking for a file with
        // the .py suffix.
        //
        // But first we need to have plugin that loads python plugins.
        // Sounds indirect and it is.  We needed to keep quickstream from
        // requiring python.  Python is optional.  quickstream can run
        // without python being installed.
        if(!(dlhandle = loader->loadControllerScript(path, argc, argv)))
            goto cleanup;
    }

    if(!dlhandle) {
        ERROR("Failed to dlopen(\"%s\",): %s", path, dlerror());
        goto cleanup;
    }

    c->dlhandle = dlhandle;

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

    ASSERT(qsDictionaryInsert(app->controllers, c->name, c, &d) == 0);
    DASSERT(d);
    qsDictionarySetFreeValueOnDestroy(d,
            (void (*)(void *)) DictionaryDestroyController);

    // Add this controller to the last of Apps doubly linked list
    //
    if(app->first) {
        DASSERT(app->last);
        app->last->next = c;
        c->prev = app->last;
    } else {
        DASSERT(app->last == 0);
        app->first = c;
    }
    app->last = c;

    c->parameters = qsDictionaryCreate();


    // We need thread specific data to tell what controller this is when
    // the controller API is called.
    CHECK(pthread_once(&keyOnce, MakeKey));


    if(construct) {

        // This needs to be re-entrant code.
        void *oldController = pthread_getspecific(_qsControllerKey);
        CHECK(pthread_setspecific(_qsControllerKey, c));

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
        CHECK(pthread_setspecific(_qsControllerKey, oldController));

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
            if(ret < 0)
                return 0; // failure returns Null pointer.
            else
                // The module did its' thing and is effectively unloading
                // itself.
                return QS_UNLOADED;
        }
    }


    INFO("Successfully loaded module Controller %s with name \"%s\"",
            path, c->name);
    free(path);

    return c; // success

cleanup:

    DASSERT(c);
    if(path)
        free(path);
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
        c = pthread_getspecific(_qsControllerKey);
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

    for(struct QsStream *s=c->app->streams; s; s = s->next)
        for(struct QsFilter *f=s->filters; f; f = f->next) {
            if(f->preInputCallbacks)
                qsDictionaryRemove(f->preInputCallbacks, c->name);
            if(f->postInputCallbacks)
                qsDictionaryRemove(f->postInputCallbacks, c->name);
        }


    DSPEW("Unloading controller \"%s\"", c->name);

    if(qsDictionaryRemove(c->app->controllers, c->name)) {
        ERROR("Controller \"%s\" not found in app", c->name);
        return -1; // error
    }

    return 0; // success
}
