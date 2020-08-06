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
#include <stdatomic.h>

// The public installed user interfaces:
#include "../include/quickstream/app.h"


// Private interfaces.
#include "./debug.h"
#include "Dictionary.h"
#include "./qs.h"
#include "./filterList.h"


uint32_t _qsAppCount = 0;


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

    app->type = _QS_APP_TYPE;
    app->id = _qsAppCount++;

    app->scriptControllerLoaders = qsDictionaryCreate();
    DASSERT(app->scriptControllerLoaders);
    app->controllers = qsDictionaryCreate();
    DASSERT(app->controllers);

    return app;
}


int qsAppDestroy(struct QsApp *app) {

    DASSERT(app);
    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    DASSERT(app->controllers);

    // Destroy the streams.  We assume they are not flowing.
    while(app->streams) qsStreamDestroy(app->streams);

    // The SetFreeValueOnDestroy callbacks will cleanup
    // all the controllers.
    qsDictionaryDestroy(app->controllers);

    // The SetFreeValueOnDestroy callbacks will cleanup
    // all the scriptControllerLoaders.
    qsDictionaryDestroy(app->scriptControllerLoaders);

#ifdef DEBUG
    memset(app, 0, sizeof(*app));
#endif
    // Now cleanup this app.
    free(app);
    return 0;
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
//
// This function calls itself.
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
                "color=\"red\", label=\"buffer %"
                PRIu32 "\"];"
                " output_%p;\n",
                numBuffers, output);

        // Draw line output -> buffer with label of output port
        fprintf(file, "    "
                "\"%s_output_%" PRIu32 "\" -> output_%p"
                " [label=\"%" PRIu32 "\"];\n",
                filter->name, i, output, i);

        if(output->prev)
            // Draw line from pass-through to origin output
            fprintf(file, "    "
                "output_%p -> output_%p "
                // This constraint=false makes it so that this line/edge
                // does not have a string that pulls the graph
                // constellation.
                "[color=red, constraint=false, "
                "label=\"pass through\", fontsize=10.0];\n",
                output->prev, output);

        DASSERT(output->numReaders, "");
        DASSERT(output->readers, "");

        for(uint32_t j=0; j<output->numReaders; ++j) {
            struct QsReader *reader = &output->readers[j];
            DASSERT(reader, "");
            //
            // Draw line buffer -> input;
            fprintf(file, "    "
                    "output_%p -> "
                    "\"%s\" [label=\"%" PRIu32 "\"];\n",
                    output,
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
    struct QsFilter *f = 0;

    for(struct QsStream *s = app->streams; s; s = s->next) {
        uint32_t numConnections;
        for(f = s->filters; f; f = f->next) {
            numConnections = s->numConnections;
            for(struct QsConnection *c = s->connections; numConnections;
                    --numConnections, ++c)
                if(f == c->to || f == c->from)
                    break;
            if(numConnections == 0)
                // We have no connection for filter f.
                break;
        }
        if(f)
            // We have no connection for filter f.
            break;
    }
        
    if(f) {
        // We have at least one unconnected filter in this app
        fprintf(file, "\n"
                "  subgraph cluster_%" PRIu32 " {\n"
                "    label=\" unconnected filters \";\n\n",
                clusterNum++);

        uint32_t s_num = 0;
        for(struct QsStream *s = app->streams; s; s = s->next) {
            uint32_t numConnections;
            bool gotOne = false;
            for(f = s->filters; f; f = f->next) {
                numConnections = s->numConnections;
                for(struct QsConnection *c = s->connections; numConnections;
                        --numConnections, ++c)
                    if(f == c->to || f == c->from)
                        break;
                if(numConnections == 0) {
                    if(!gotOne) {
       
                        // We have at least one unconnected filter in this
                        // app
                        fprintf(file, "\n"
                                "    subgraph cluster_%" PRIu32 " {\n"
                                "       label=\" stream %" PRIu32 " \";\n\n",
                                clusterNum++, s_num);
                        gotOne = true;
                    }
                    fprintf(file, "        \"%s\" [label=\"%s\"];\n",
                            f->name, f->name);
                }
            }
            if(gotOne)
                fprintf(file, "     }\n");
            ++s_num;
        }

        fprintf(file, "    }\n");
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
        // To see the dot file in the terminal:
        //execlp("cat", "cat", (const char *) 0);

        execlp("display", "display", (const char *) 0);
        WARN("execlp(\"%s\", \"%s\", 0) failed", "display", "display");
        exit(1);
    }

    return ret; // success ret=0
}
