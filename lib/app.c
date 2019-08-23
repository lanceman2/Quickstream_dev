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
#include "./filterList.h"



struct QsApp *qsAppCreate(void) {

    struct QsApp *app = calloc(1, sizeof(*app));

    return app;
}


int qsAppDestroy(struct QsApp *app) {

    DASSERT(app, "");

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


int qsAppPrintDotToFile(struct QsApp *app, FILE *file) {

    DASSERT(app, "");
    DASSERT(file, "");

    fprintf(file, "digraph {\n"
        "  label=\"quickstream app\";\n");

    uint32_t clusterNum = 0; // Dot cluster counter
 
    // Look for unconnected filters in this app:
    struct QsFilter *f=app->filters;
    for(struct QsFilter *f=app->filters; f; f = f->next)
        if(!f->stream)
            break;
    if(f) {
        // We have at least one unconnected filter in this app
        fprintf(file, "\n"
                "  subgraph cluster_%" PRIu32 " {\n"
                "    label=\" unconnected filters \";\n\n",
                clusterNum++);
        
        for(struct QsFilter *f=app->filters; f; f=f->next)
            if(!f->stream)
                fprintf(file, "    \"%s\";\n", f->name);

        fprintf(file, "  }\n");
    }


    uint32_t sNum = 0; // stream counter

    for(struct QsStream *s = app->streams; s; s = s->next) {
        fprintf(file, "\n"
                "  subgraph cluster_%" PRIu32 " {\n"
                "    label=\"stream %" PRIu32 "\";\n\n",
                clusterNum++, sNum);
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
