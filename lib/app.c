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
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

// The public installed user interfaces:
#include "../include/qsapp.h"


// Private interfaces.
#include "./qsapp.h"
#include "./debug.h"
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

#ifdef DEBUG
    memset(app, 0, sizeof(*app));
#endif

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
        // Because they requested a particular name and the name is
        // already taken, we can fail here.
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
    
    // It should be a full path.
    DASSERT(path[0] == '/', "");

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
    f->dlhandle = handle;

    // Clear the dl error
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

    DSPEW();

    f->app = app;
    DSPEW();

    return f; // success

cleanup:

    // failure mode.
    //
    RemoveFilterFromList(app, f);
    free(path);
    return 0; // failure
}


int qsFilterUnload(struct QsFilter *f) {

    DASSERT(f, "");
    DASSERT(f->app, "");

    RemoveFilterFromList(f->app, f);

    return 0; // success
}

int qsAppPrintDotToFile(struct QsApp *app, FILE *file) {

    DASSERT(app, "");
    DASSERT(file, "");

    fprintf(file, "digraph {\n"
        "  label=\"quickstream app\";\n");

    uint32_t sNum = 0;

    for(struct QsStream *s = app->streams; s; s = s->next) {
        fprintf(file, "\n"
                "  subgraph cluster_%" PRIu32 " {\n"
                "    label=\"stream %" PRIu32 "\";\n\n",
                sNum, sNum);
        for(uint32_t i=0; i<s->numConnections; ++i) {
            DASSERT(s->from[i], "");
            DASSERT(s->from[i]->name, "");
            DASSERT(s->to[i], "");
            DASSERT(s->to[i]->name, "");
            fprintf(file, "    \"%s\" -> \"%s\";\n",
                    s->from[i]->name, s->to[i]->name);
        }
        fprintf(file, "  }\n");
        ++sNum;
    }

    fprintf(file, "}\n");


    DSPEW();

    return 0; // success
}

int qsAppDisplayFlowImage(struct QsApp *app, bool waitForDisplay) {

    DASSERT(app, "");

    int fd[2];
    int ret = 0;

    if(pipe(fd) != 0) {
        ERROR("pipe() failed");
        return 1;
    }

    pid_t pid = fork();

    if(pid < 0) {
        ERROR("fork() failed");
        return 1;
    }

    if(pid) {
        //
        // I'm the parent
        //
        close(fd[0]);
        FILE *f = fdopen(fd[1], "w");
        if(!f) {
            ERROR("fdopen() failed");
            return 1;
        }

        if(qsAppPrintDotToFile(app, f))
            ret = 1; // failure
        fclose(f);

        if(waitForDisplay) {
            int status = 0;
            if(waitpid(pid, &status, 0) == -1) {
                WARN("waitpid(%u,,0) failed", pid);
                ret = 1;
            }
            INFO("waitpid() returned status=%d", status);
        }
    } else {
        //
        // I'm the child
        //
        DASSERT(pid == 0, "I'm I a child process or not");
        close(fd[1]);
        if(fd[0] != 0) {
            // Use dup2() to turn the read part of the pipe, fd[0], into
            // the stdin, fd=0, of the child process.
            if(dup2(fd[0], 0)) { 
                WARN("dup2(%d,%d) failed", fd[0], 0);
                exit(1);
            }
            close(fd[0]);
        }
        execlp("display", "display", (const char *) 0);
        WARN("execlp(\"%s\", \"%s\", 0) failed", "display", "display");
        exit(1);
    }

    return ret; // success ret=0
}
