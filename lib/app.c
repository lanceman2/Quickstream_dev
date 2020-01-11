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
#include "../include/quickstream/app.h"


// Private interfaces.
#include "./qs.h"
#include "./debug.h"
#include "./filterList.h"


#ifdef DEBUG
pthread_t _qsMainThread = 0;
#endif

pthread_key_t _qsKey = 0;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static void make_key(void) {
    CHECK(pthread_key_create(&_qsKey, 0));
};


struct QsApp *qsAppCreate(void) {

#ifdef DEBUG
    if(!_qsMainThread)
        // We define this to be the main thread.
        _qsMainThread = pthread_self();
#endif

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");

    struct QsApp *app = calloc(1, sizeof(*app));
    ASSERT(app, "calloc(1,%zu) failed", sizeof(*app));

    // key used for pthread_getspecific() and pthread_setspecific()
    // a thread calls before filter::input() in the multi-threaded
    // flow/run case.  We don't know if we are multi-threaded until
    // flow start.
    CHECK(pthread_once(&key_once, make_key));

    return app;
}


int qsAppDestroy(struct QsApp *app) {

    DASSERT(app);
    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    
    // Destroy the streams.  We assume they are not flowing.
    while(app->streams) qsStreamDestroy(app->streams);

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


// In this case we print from the stream (QsStream) list of connections.
// This stream->connections list is an intermediate step in setting up the
// flow graph; it is not used when the stream is flowing.
//
// Returns clusterNum that is used for referencing subgraph clusters in
// the dot output.
//
static inline uint32_t
PrintStreamOutline(struct QsStream *s,
        uint32_t sNum /*stream number*/,
        uint32_t clusterNum,
        FILE *file) {

    DASSERT(s->numConnections || s->connections == 0, "");

    fprintf(file, "\n"
            "  subgraph cluster_%" PRIu32 " {\n"
            "    label=\"stream %" PRIu32 "\";\n\n",
            clusterNum++, sNum);
    for(uint32_t i=0; i<s->numConnections; ++i) {
        DASSERT(s->connections[i].from, "");
        DASSERT(s->connections[i].from->name, "");
        DASSERT(s->connections[i].to, "");
        DASSERT(s->connections[i].to->name, "");
        fprintf(file, "    \"%s\" -> \"%s\";\n",
                s->connections[i].from->name, s->connections[i].to->name);
    }
    fprintf(file, "  }\n");
    return clusterNum;
}


static uint32_t
PrintStreamFilter1(struct QsFilter *filter, uint32_t clusterNum,
        FILE *file) {

    filter->mark = false; // Mark this filter looked at.

    fprintf(file, "\n"
        "    subgraph cluster_%" PRIu32 " {\n"
        "      label=\"%s\";\n\n",
        clusterNum++, filter->name);

    fprintf(file, "      \"%s\" [label=\"input()\"];\n", filter->name);

    for(uint32_t i=0; i<filter->numOutputs; ++i)
        fprintf(file, "      node [shape=\"box\", "
                "label=\"output %" PRIu32 "\"]; "
                "\"%s_output_%" PRIu32 "\"; \n" , i, filter->name, i);

    fprintf(file, "    }\n");


    // Recurse
    for(uint32_t i=0; i<filter->numOutputs; ++i) {
        struct QsOutput *output = &filter->outputs[i];
        DASSERT(output, "");
        DASSERT(output->readers || output->numReaders == 0, "");

        for(uint32_t j=0; j<output->numReaders; ++j)
            // Skip unmarked filters.
            if(output->readers[j].filter->mark)
                // Recurse
                clusterNum = PrintStreamFilter1(output->readers[j].filter,
                        clusterNum, file);
    }

    return clusterNum;
}


// This prints the connections from the outputs through the buffer to the
// inputs.  At this point the outputs have a location in the filters in
// the graph.
static inline uint32_t
PrintStreamFilterBuffer(struct QsFilter *filter, uint32_t numBuffers,
        FILE *file) {

    DASSERT(filter,"");
    DASSERT(filter->numOutputs || filter->outputs == 0, "");

    for(uint32_t i=0; i<filter->numOutputs; ++i) {

        struct QsOutput *output = &filter->outputs[i];

        // Define a dot shape for the buffer.
        //
        fprintf(file, "\n    node [shape=\"parallelogram\","
                "color=\"red\", label=\"buffer %" PRIu32 "\"];"
                " \"%s_writer_%" PRIu32 "\";\n",
                numBuffers,
                filter->name, numBuffers);

        // Draw output -> buffer;
        fprintf(file,"    "
                "\"%s_output_%" PRIu32 "\" -> "
                "\"%s_writer_%" PRIu32 "\""
                " [label=\"%" PRIu32 "\"];\n",
                filter->name, i,
                filter->name, numBuffers, i);

        DASSERT(output->numReaders, "");
        DASSERT(output->readers, "");

        for(uint32_t j=0; j<output->numReaders; ++j) {
            struct QsReader *reader = &output->readers[j];
            DASSERT(reader, "");
            //
            // Draw buffer -> input;
            fprintf(file,"    "
                    "\"%s_writer_%" PRIu32 "\" -> "
                    "\"%s\" [label=\"%" PRIu32 "\"];\n",
                    filter->name, numBuffers,
                    reader->filter->name,
                    reader->inputPortNum);
        }

        // Count this buffer:
        //
        ++numBuffers;
    }

    filter->mark = false; // Mark this filter looked at (graphed).

    // Recurse
    for(uint32_t i=0; i<filter->numOutputs; ++i) {
        struct QsOutput *output = &filter->outputs[i];

        for(uint32_t j=0; j<output->numReaders; ++j)
            // Skip unmarked filters.
            if(output->readers[j].filter->mark)
                // Recurse
                numBuffers = PrintStreamFilterBuffer(
                        output->readers[j].filter,
                        numBuffers, file);
    }

    return numBuffers; // So we tally up the number of buffers.
}


static inline uint32_t
PrintStreamDetail(struct QsStream *s,
        uint32_t sNum /*stream number*/,
        uint32_t clusterNum,
        FILE *file) {

    DASSERT(s->sources, "");

    // TODO: add an image of the buffers so we may see how they are
    // shared.

    fprintf(file, "\n"
            "  subgraph cluster_%" PRIu32 " {\n"
            "    label=\"stream %" PRIu32 "\";\n",
            clusterNum++, sNum);

    // We do this in 2 passes:

    /////////// pass 1
    // Mark all the filters as needing to be looked at.
    StreamSetFilterMarks(s, true);
    for(uint32_t i=0; i<s->numSources; ++i)
        clusterNum = PrintStreamFilter1(s->sources[i], clusterNum, file);

    /////////// pass 2
    // Mark all the filters as needing to be looked at.
    StreamSetFilterMarks(s, true);
    for(uint32_t i=0; i<s->numSources; ++i)
        PrintStreamFilterBuffer(s->sources[i], 0, file);

    fprintf(file, "  }\n");

    return clusterNum;
}


int qsAppPrintDotToFile(struct QsApp *app, enum QsAppPrintLevel l,
        FILE *file) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    DASSERT(app, "");
    DASSERT(file, "");


    fprintf(file, "digraph {\n"
        "  label=\"quickstream app\";\n");

    uint32_t clusterNum = 0; // Dot cluster counter
 
    // Look for unconnected filters in this app:
    struct QsFilter *f=app->filters;
    for(; f; f = f->next)
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
        if(l == QSPrintOutline || !s->sources)
            clusterNum = PrintStreamOutline(s, sNum++, clusterNum, file);
        else
            // More verbose print.
            clusterNum = PrintStreamDetail(s, sNum++, clusterNum, file);
    }

    fprintf(file, "}\n");

    return 0; // success
}


int qsAppDisplayFlowImage(struct QsApp *app, enum QsAppPrintLevel l,
        bool waitForDisplay) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
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

        if(qsAppPrintDotToFile(app, l, f))
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
