#ifndef _GNU_SOURCE
// For the GNU version of basename() that never modifies its argument.
// and For the GNU version of dlmopen()
#  define _GNU_SOURCE
#endif
#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

// The public installed user interfaces:
#include "../include/qsapp.h"


// Private interfaces.
#include "./qs.h"
#include "./debug.h"
#include "./filterList.h"
#include "./GetPath.h"



int qsFilterPrintHelp(const char *filterName, FILE *f) {

    if(f == 0) f = stderr;

    char *path = GetPluginPath("filters", filterName);
    // It should be a full path now.
    DASSERT(path[0] == '/', "");

    void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);

    if(!handle) {
        ERROR("Failed to dlopen(\"%s\",): %s", path, dlerror());
        free(path);
        return -1; // error
    }

    dlerror(); // clear error
    void (*help)(FILE *) = dlsym(handle, "help");
    char *err = dlerror();
    if(err) {
        // We must have a input() function.
        ERROR("Module at path=\"%s\" no input() provided:"
                " dlsym(\"help\") error: %s", path, err);
        free(path);
        dlclose(handle);
        return -1; // error
    }

    fprintf(f, "\nfilter path=%s\n\n", path);
    help(f);

    dlclose(handle);
    free(path);
    return 0; // success
}


struct QsFilter *qsAppFilterLoad(struct QsApp *app,
        const char *fileName, const char *_loadName,
        int argc, const char **argv) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
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
        // loadName = "foo/filter/test/filename.so"
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
            // loadName = "/filename"

            // If the next directory in the path is not named
            // "/filter/" we add it to the loadName.
            const size_t flen = strlen(DIR_STR "filters");
            while(!(loadName - s > flen &&
                    strncmp(loadName-flen, DIR_STR "filters", flen)
                    == 0) &&
                    loadName > s + 2) {
                --loadName;
                while(loadName != s && *loadName != DIR_CHAR)
                    loadName--;
                // loadName = "/test/filename"
            }
            if(*loadName == DIR_CHAR)
                ++loadName;
        }
    } else if(FindFilterNamed(app, _loadName)) {
        //
        // Because they requested a particular name and the name is
        // already taken, we can fail here.
        //
        ERROR("Filter name \"%s\" is in use already", _loadName);
        return 0;
    }

    char *path = GetPluginPath("filters", fileName);


    void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);

    if(!handle) {
        ERROR("Failed to dlopen(\"%s\",): %s", path, dlerror());
        free(path);
        return 0;
    }

    if(FindFilter_viaHandle(app, handle)) {
        //
        // This DSO (dynamic shared object) file is already loaded.  So we
        // must copy the DSO file to a temp file and load that.  Otherwise
        // we will have just the one plugin loaded, but referred to by two
        // (or more) filters, which is not what we want.  The temp file
        // will be automatically removed when the process exits.
        //

        if(dlclose(handle)) {
            ERROR("dlclose() of %s failed: %s", path, dlerror());
            free(path);
            return 0;
        }

        INFO("DSO from %s was loaded before, loading a copy", path);
        char tmpFilename[63];
        strcpy(tmpFilename, "/tmp/qs_XXXXXX.so");
        int tmpFd = mkstemps(tmpFilename, 3);
        if(tmpFd < 0) {
            ERROR("mkstemp() failed");
            free(path);
            return 0;
        }
        DSPEW("made temporary file: %s", tmpFilename);
        int dso = open(path, O_RDONLY);
        if(dso < 0) {
            ERROR("open(\"%s\", O_RDONLY) failed", path);
            close(tmpFd);
            free(path);
            unlink(tmpFilename);
            return 0;
        }
        const size_t len = 1024;
        uint8_t buf[len];
        ssize_t rr = read(dso, buf, len);
        while(rr > 0) {
            ssize_t wr;
            size_t bw = 0;
            while(rr > 0) {
                wr = write(tmpFd, &buf[bw], rr);
                if(wr < 1) {
                    ERROR("Failed to write to %s", tmpFilename);
                    close(tmpFd);
                    close(dso);
                    free(path);
                    unlink(tmpFilename);
                    return 0;
                }
                rr -= wr;
                bw += wr;
            }
 
            rr = read(dso, buf, len);
        }
        close(tmpFd);
        close(dso);
        chmod(tmpFilename, 0700);

        handle = dlopen(tmpFilename, RTLD_NOW | RTLD_LOCAL);
        //
        // This file is mapped to the process.  No other process will have
        // access to this tmp file after the following unlink() call.
        //
        if(unlink(tmpFilename))
            // There is no big reason to fuss to much.
            WARN("unlink(\"%s\") failed", tmpFilename);

        if(!handle) {
            ERROR("dlopen(\"%s\",) failed: %s", tmpFilename, dlerror());
            free(path);
            return 0;
        }
    }


    struct QsFilter *f = AllocAndAddToFilterList(app, loadName);

    // If "construct", "destroy", "start", or "stop"
    // are not present, that's okay, they are optional.
    int (* construct)(int argc, const char **argv) =
        dlsym(handle, "construct");
    f->start = dlsym(handle, "start");
    f->stop = dlsym(handle, "stop");

    dlerror(); // clear error
    // "input()" is not optional.
    f->input = dlsym(handle, "input");
    char *err = dlerror();
    if(err) {
        // We must have a input() function.
        ERROR("no input() provided: dlsym(\"input\") error: %s", err);
        goto cleanup;
    }


    dlerror(); // clear error
    dlsym(handle, "help");
    err = dlerror();
    if(err) {
#ifdef QS_FILTER_REQUIRE_HELP
        // "help()" is not optional.
        // We must have a help() function.
        ERROR("no help() provided: dlsym(\"help\") error: %s", err);
        goto cleanup;
#else
        // help() is optional.
        WARN("Filter \"%s\" module from file=\"%s\" does not provide a "
                "help() function.",
                f->name, path);
#endif
    }


    f->app = app;
    f->dlhandle = handle;


    if(construct) {

        // This code is only run in one thread, so this is not necessarily
        // required to be thread safe, but it is and must be re-entrant.
        // Currently this is not thread-safe; _qsConstructFilter is
        // global.
        struct QsFilter *old_qsConstructFilter = _qsConstructFilter;

        // In this construct() call this function, qsAppFilterLoad(), may
        // be called, so this function must be re-entrant.

        _qsConstructFilter = f;
        // When this construct() is called this filter struct is "all
        // setup" and in the app object.  Therefore if this filter module
        // is a super-module that loads other filters and adds
        // connections, it will work.
        int ret = construct(argc, argv);

        _qsConstructFilter = old_qsConstructFilter;
        
        if(ret) {
            // Failure.
            //
            ERROR("filter \"%s\" construct() failed", f->name);
            goto cleanup;
        }
        //else Success.
    }

    INFO("Successfully loaded module Filter %s with name \"%s\"",
            path, f->name);
    free(path);

    f->args = calloc(1, sizeof(*f->args));
    ASSERT(f->args, "calloc(1,%zu) failed", sizeof(*f->args));


    return f; // success

cleanup:

    // failure mode.
    //

    DestroyFilter(app, f);
    free(path);
    return 0; // failure
}


// maxThread that may run in the current filter.
void qsSetThreadSafe(uint32_t maxThreads) {

    DASSERT(maxThreads, "maxThreads cannot be 0");
    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    struct QsFilter *f = _qsConstructFilter;
    ASSERT(f,"qsSetThreadSafe() not called in construct()");
    DASSERT(f->args, "");
    DASSERT(f->args->buffers == 0, "");

    f->maxThreads = maxThreads;
    free(f->args);
    f->args = calloc(maxThreads, sizeof(*f->args));
    ASSERT(f->args, "calloc(%" PRIu32 ",%zu) failed",
            maxThreads, sizeof(*f->args));

    // If a filter can be called with multiple thread than we need
    // a mutex.

    f->mutex = calloc(1, sizeof(*f->mutex));
    ASSERT(f->mutex, "calloc(1,%zu) failed", sizeof(*f->mutex));
    CHECK(pthread_mutex_init(f->mutex, 0));
}}


int qsFilterUnload(struct QsFilter *f) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    ASSERT(_qsStartFilter == 0,
            "This cannot be called in start() or stop()");

    DASSERT(f, "");
    DASSERT(f->app, "");

    DestroyFilter(f->app, f);

    return 0; // success
}
