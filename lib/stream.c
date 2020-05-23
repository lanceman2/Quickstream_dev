#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dlfcn.h>
#include <stdatomic.h>

// The public installed user interfaces:
#include "../include/quickstream/app.h"
#include "../include/quickstream/filter.h"

// Private interfaces.
#include "./debug.h"
#include "Dictionary.h"
#include "./qs.h"
#include "filterList.h"
#include "stream.h"



uint32_t qsFilterStreamId(const struct QsFilter *f) {

    DASSERT(f);
    DASSERT(f->stream);

    return f->stream->id;
}


struct QsStream *qsAppStreamCreate(struct QsApp *app) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    DASSERT(app);

    struct QsStream *s = calloc(1, sizeof(*s));
    ASSERT(s, "calloc(1, %zu) failed", sizeof(*s));

    // Add this stream to the end of the app list
    //
    struct QsStream *S = app->streams;
    if(S) {
        while(S->next) S = S->next;
        S->next = s;
    } else
        app->streams = s;

    s->type = _QS_STREAM_TYPE;
    s->app = app;
    s->flags = _QS_STREAM_DEFAULTFLAGS;
    s->id = app->streamCount++;

    s->dict = qsDictionaryCreate();

    return s;
}


void qsStreamAllowLoops(struct QsStream *s, bool doAllow) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    DASSERT(s);
    DASSERT(!(s->flags & _QS_STREAM_START), "We are starting");
    DASSERT(!(s->flags & _QS_STREAM_STOP), "We are stopping");

    // s->sources != 0 at start and stop and this should be a good enough
    // check, if this code is consistent.
    ASSERT(s->sources == 0,
            "The stream is in the wrong mode to call this now.");


    if(doAllow)
        s->flags |= _QS_STREAM_ALLOWLOOPS;
    else
        s->flags &= ~_QS_STREAM_ALLOWLOOPS;
}


static inline void CleanupStream(struct QsStream *s) {

    DASSERT(s);
    DASSERT(s->app);
    DASSERT(s->numConnections || s->connections == 0);

    if(s->sources) {
        DASSERT(s->numSources);
#ifdef DEBUG
        memset(s->sources, 0, sizeof(*s->sources)*s->numSources);
#endif
        free(s->sources);
    }

    if(s->numConnections) {

        DASSERT(s->connections);
#ifdef DEBUG
        memset(s->connections, 0,
                sizeof(*s->connections)*s->numConnections);
#endif
        free(s->connections);
        s->connections = 0;
        s->numConnections = 0;
    }
#ifdef DEBUG
    memset(s, 0, sizeof(*s));
#endif
    free(s);
}



void qsFiltersConnect(
        struct QsFilter *from, struct QsFilter *to,
        uint32_t fromPortNum, uint32_t toPortNum) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");

    DASSERT(from);

    struct QsStream *s = from->stream;
    DASSERT(s);
    DASSERT(s->app);
    ASSERT(from != to, "a filter cannot connect to itself");
    DASSERT(to);
    DASSERT(to->app);
    DASSERT(from->app == s->app, "wrong app");
    DASSERT(to->app == s->app, "wrong app");
    ASSERT(to->stream == s,
            "filter cannot be part of another stream");
    DASSERT(!(s->flags & _QS_STREAM_START), "We are starting");
    ASSERT(!(s->flags & _QS_STREAM_STOP), "We are stopping");
    ASSERT(s->sources == 0,
            "stream is in the wrong mode to call this now.");


    // Grow the connections array:
    //
    uint32_t numConnections = s->numConnections;
    s->connections = realloc(s->connections,
            (numConnections+1)*sizeof(*s->connections));
    ASSERT(s->connections, "realloc(,%zu) failed",
            (numConnections+1)*sizeof(*s->connections));
    //
    s->connections[numConnections].from = from;
    s->connections[numConnections].to = to;
    s->connections[numConnections].fromPortNum = fromPortNum;
    s->connections[numConnections].toPortNum = toPortNum;
    //
    s->connections[numConnections].from->stream = s;
    s->connections[numConnections].to ->stream = s;
    //
    ++s->numConnections;
}



static inline void RemoveConnection(struct QsStream *s, uint32_t i) {

    DASSERT(s);
    DASSERT(i < -1);
    DASSERT(s->numConnections > i);

    // This is how many connections there will be after this function
    --s->numConnections;

    struct QsFilter *from = s->connections[i].from;
    struct QsFilter *to = s->connections[i].to;

    DASSERT(from);
    DASSERT(to);
    DASSERT(from->stream == s);
    DASSERT(to->stream == s);
    // Outputs should not be allocated yet.
    DASSERT(from->outputs == 0);
    DASSERT(to->outputs == 0);
    DASSERT(from->numOutputs == 0);
    DASSERT(to->numOutputs == 0);

    if(s->numConnections == 0) {

        // Singular case.  We have no connections anymore.
        //
        free(s->connections);
        s->connections = 0;
        return;
    }


    // At index i shift all the connections back one, overwriting the i-th
    // one.
    for(;i<s->numConnections; ++i)
        s->connections[i] = s->connections[i+1];

    // Shrink the connections arrays by one
    s->connections = realloc(s->connections,
            (s->numConnections)*sizeof(*s->connections));
    ASSERT(s->connections, "realloc(,%zu) failed",
            (s->numConnections)*sizeof(*s->connections));
}



int StreamRemoveFilterConnections(struct QsStream *s,
        struct QsFilter *f) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    DASSERT(s);
    DASSERT(f);
    DASSERT(f->stream == s);
    DASSERT(s->numConnections || s->connections == 0);
    DASSERT(!(s->flags & _QS_STREAM_START), "We are starting");
    ASSERT(!(s->flags & _QS_STREAM_STOP), "We are stopping");
    ASSERT(s->sources == 0,
            "stream is in the wrong mode to call this now.");

    bool gotOne = false;

    for(uint32_t i=0; i<s->numConnections;) {
        DASSERT(s->connections);
        DASSERT(s->connections[i].to);
        DASSERT(s->connections[i].from);
        DASSERT(s->connections[i].to != s->connections[i].from);
        //
        if(s->connections[i].from == f || s->connections[i].to == f) {
            RemoveConnection(s, i);
            // s->numConnections just got smaller by one
            // and the current index points to the next
            // element, so we do not increment i.
            gotOne = true;
        } else
            // regular increment i to next element:
            ++i;
    }

    // return 0 == success found and removed a connection, 1 to fail.
    return gotOne?0:1;
}



void qsStreamDestroy(struct QsStream *s) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    DASSERT(s);
    DASSERT(s->app);
    DASSERT(s->numConnections || s->connections == 0);
    DASSERT(!(s->flags & _QS_STREAM_START), "We are starting");
    DASSERT(!(s->flags & _QS_STREAM_STOP), "We are stopping");

    FreeRunResources(s);

    // Cleanup filters in this list
    struct QsFilter *f = s->filters;
    while(f) {
        struct QsFilter *nextF = f->next;
        // Destroy this filter f.
        DestroyFilter(s, f);
        f = nextF;
    }

    qsDictionaryDestroy(s->dict);

    // Find and remove this stream from the app.
    //
    struct QsStream *S = s->app->streams;
    struct QsStream *prev = 0;
    while(S) {
        if(S == s) {

            if(prev)
                prev->next = s->next;
            else
                s->app->streams = s->next;

            CleanupStream(s);
            break;
        }
        prev = S;
        S = S->next;
    }

    DASSERT(S, "stream was not found in the app object");
}



int qsStreamForEachFilter(void *sOrA,
        int (*callback)(struct QsStream *stream,
            struct QsFilter *filter, void *userData),
        void *userData) {


    struct QsApp *app = sOrA;
    struct QsStream *s = 0;

    if(!sOrA) {
        // This must be a controller calling this.
        struct QsController *c = pthread_getspecific(_qsControllerKey);
        DASSERT(c);
        if(c) {
            DASSERT(c->name);
            DASSERT(c->name[0]);
            DASSERT(c->mark == _QS_IN_CCONSTRUCT ||
                    c->mark == _QS_IN_CDESTROY ||
                    c->mark == _QS_IN_PRESTART ||
                    c->mark == _QS_IN_POSTSTART ||
                    c->mark == _QS_IN_PRESTOP ||
                    c->mark == _QS_IN_POSTSTOP);
            app = c->app;
        } else {
            ERROR("No valid controller, app, or stream found");
            return -1;
        }
    }
    
    if(app->type != _QS_APP_TYPE) {

        DASSERT(app->type == _QS_STREAM_TYPE);

        app = 0;
        s = sOrA;
    } else
        s = app->streams;


    int ret = 0;

    for(;s ; s = (app)?(s->next):0)
        for(struct QsFilter *f=s->filters; f; f = f->next) {
            ++ret;
            if(callback(s, f, userData))
                return ret; // success
        }

    return ret; // success
}
