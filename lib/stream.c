#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// The public installed user interfaces:
#include "../include/qsapp.h"


// Private interfaces.
#include "./qsapp.h"
#include "./debug.h"


struct QsStream *qsAppStreamCreate(struct QsApp *app) {
    
    DASSERT(app, "");

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

    s->app = app;

    return s;
}


int qsStreamDestroy(struct QsStream *s) {

    DASSERT(s, "");
    DASSERT(s->app, "");

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
            if(s->sources) free(s->sources);
            if(s->numConnections) {

                DASSERT(s->from, "");
                DASSERT(s->to, "");
#ifdef DEBUG
                memset(s->from, 0, sizeof(*s->from)*s->numConnections);
                memset(s->to, 0, sizeof(*s->to)*s->numConnections);
#endif
                free(s->from);
                free(s->to);
            }
#ifdef DEBUG
            memset(s, 0, sizeof(*s));
#endif
            free(s);
            break;
        }
        prev = S;
        S = S->next;
    }

    DASSERT(S, "stream was not found in the app object");

    return 0; // success
}


int qsStreamConnectFilters(struct QsStream *s,
        struct QsFilter *from, struct QsFilter *to) {

    DASSERT(s, "");
    DASSERT(s->app, "");
    DASSERT(from, "");
    DASSERT(to, "");
    DASSERT(from->app, "");
    DASSERT(to->app, "");
    DASSERT(from->app == s->app, "wrong app");
    DASSERT(to->app == s->app, "wrong app");
    DASSERT(to->stream == s || !to->stream,
            "filter cannot be part of another stream");
    DASSERT(from->stream == s || !from->stream,
            "filter cannot be part of another stream");

    s->from = realloc(s->from, (s->numConnections+1)*sizeof(*s->from));
    ASSERT(s->from, "realloc(,%zu) failed",
            (s->numConnections+1)*sizeof(*s->from));
    s->from[s->numConnections] = from;
    s->to = realloc(s->to, (s->numConnections+1)*sizeof(*s->to));
    ASSERT(s->to, "realloc(,%zu) failed",
            (s->numConnections+1)*sizeof(*s->to));
    s->to[s->numConnections] = to;
    ++s->numConnections;

    from->stream = s;
    to->stream = s;

    return 0; // success
}
