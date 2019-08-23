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

static inline void CleanupStream(struct QsStream *s) {

    DASSERT(s, "");
    DASSERT(s->app, "");
    DASSERT(s->numConnections || (
        s->from == 0 && s->to == 0), "");

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

        s->from = s->to = 0;
        s->numConnections = 0;
    }
#ifdef DEBUG
    memset(s, 0, sizeof(*s));
#endif
    free(s);
}


int qsStreamDestroy(struct QsStream *s) {

    DASSERT(s, "");
    DASSERT(s->app, "");
    DASSERT(s->numConnections || (
        s->from == 0 && s->to == 0), "");

    // Remove this stream from all the filters in the listed connections.
    for(uint32_t i=0; i<s->numConnections; ++i) {
        DASSERT(s->from[i], "");
        DASSERT(s->to[i], "");
        DASSERT(s->from[i]->stream == s || s->from[i]->stream == 0, "");
        DASSERT(s->to[i]->stream == s || s->to[i]->stream == 0, "");

        s->from[i]->stream = 0;
        s->to[i]->stream = 0;
    }

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

    return 0; // success
}


int qsStreamConnectFilters(struct QsStream *s,
        struct QsFilter *from, struct QsFilter *to) {

    DASSERT(s, "");
    DASSERT(s->app, "");
    DASSERT(from != to, "a filter cannot connect to itself");
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

    if(from == to) {
        ERROR("A filter cannot connect to itself");
        return 1;
    }

    // Grow the from and to arrays
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


static inline void RemoveConnection(struct QsStream *s, uint32_t i) {

    DASSERT(s, "");
    DASSERT(s->numConnections > i, "");

    // This is how many connections there will be after this function
    --s->numConnections;

    struct QsFilter *from = s->from[i];
    struct QsFilter *to = s->to[i];

    DASSERT(from, "");
    DASSERT(to, "");
    DASSERT(from->stream == s, "");
    DASSERT(to->stream == s, "");

    // At index i shift all the connections back one, overwriting the i-th
    // one.
    for(;i<s->numConnections; ++i) {
        s->from[i] = s->from[i+1];
        s->to[i] = s->to[i+1];
    }

    // Shrink the "from" and "to" arrays by one
    s->from = realloc(s->from, (s->numConnections)*sizeof(*s->from));
    ASSERT(s->from, "realloc(,%zu) failed",
            (s->numConnections)*sizeof(*s->from));
    s->to = realloc(s->to, (s->numConnections)*sizeof(*s->to));
    ASSERT(s->to, "realloc(,%zu) failed",
            (s->numConnections)*sizeof(*s->to));

    // If this "from" filter is not in this stream mark it as such.
    for(i=0; i<s->numConnections; ++i)
        if(s->from[i] == from || s->to[i] == from)
            break;
    if(i==s->numConnections)
        // This "from" filter has no stream associated with it:
        from->stream = 0;

    // If the "to" filter is not in this stream mark it as such.
    for(i=0; i<s->numConnections; ++i)
        if(s->from[i] == to || s->to[i] == to)
            break;
    if(i==s->numConnections)
        // This "to" filter has no stream associated with it:
        to->stream = 0;
}


int qsStreamRemoveFilter(struct QsStream *s, struct QsFilter *f) {

    DASSERT(s, "");
    DASSERT(f, "");
    DASSERT(f->stream == s, "");

    bool gotOne = false;

    for(uint32_t i=0; i<s->numConnections;) {
        DASSERT(s->from[i], "");
        DASSERT(s->to[i], "");
        DASSERT(s->to[i] != s->from[i], "");
        if(s->from[i] == f || s->to[i] == f) {
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


static void inline FreeSources(struct QsStream *s) {

    DASSERT(s, "");
    DASSERT(s->app, "");
    DASSERT(s->numSources, "");
    DASSERT(s->sources, "");

#ifdef DEBUG
    memset(s->sources, 0, sizeof(*s->sources)*s->numSources);
#endif

    free(s->sources);

#ifdef DEBUG
    s->sources = 0;
#endif

    s->numSources = 0;
}


static inline uint32_t CountFilterPath(struct QsStream *s,
        struct QsFilter *f, uint32_t loopCount, uint32_t maxCount) {

    if(loopCount > maxCount)
        // break the looping.  Stop recursing.  We must have already
        // started looping, because we counted more filters in the path
        // than the number of filters that exist.
        return loopCount;

    // In any path in the flow we can only have a filter traversed once.
    // If in any path a filter is traversed more than once then there will
    // be loops and if there is a loop in any part this will recurse
    // forever.
    //
    // Easier is to check if there is a filter path greater than
    // the number of total filters that can be traversed (maxCount).
    //
    for(uint32_t i=0; i<s->numConnections; ++i)
        // look for this source as a "from" filter
        if(s->from[i] == f) {
            uint32_t count =
                // recurse
                CountFilterPath(s, s->to[i], loopCount+1, maxCount);
            // this filter feeds the "to" filter.
            if(count > loopCount)
                // We want the largest filter count in all paths.
                loopCount = count;
        }

    return loopCount;
}


int qsStreamStop(struct QsStream *s) {

    DASSERT(s, "");
    DASSERT(s->app, "");

    if(s->numSources == 0)
        // The stream was not in a flow state, and none of the filter
        // callbacks where called.
        return 1;


    DASSERT(s->numSources, "");
    DASSERT(s->sources, "");


    /**********************************************************************
     *            Stage: free sources list
     *********************************************************************/
    FreeSources(s);

    return 0;
}


int qsStreamStart(struct QsStream *s) {

    DASSERT(s, "");
    DASSERT(s->app, "");

    ///////////////////////////////////////////////////////////////////////
    //                                                                   //
    //  This has a few stages in which we go through the lists, check    //
    //  things out, set things up, launch processes, launch threads,     //
    //  and then let the stream flow through the filters.                //
    //                                                                   // 
    ///////////////////////////////////////////////////////////////////////

    // Failure error return values start at -1 and go down from there;
    // like: -1, -2, -3, -4, ...

    /**********************************************************************
     *            Stage: simple checks
     *********************************************************************/


    /**********************************************************************
     *            Stage: Find source filters
     *********************************************************************/

    for(uint32_t i=0; i < s->numConnections; ++i) {

        struct QsFilter *f = s->from[i];
        uint32_t j=0;
        for(; j<s->numConnections; ++j) {
            if(i == j) {
                DASSERT(s->from[i] != s->to[j],
                        "filter cannot connect to itself");
                continue;
            }
            if(s->to[j] == f)
                // f is not a source
                break;
        }
        if(!(j < s->numConnections)) {
            // Add f to the source list.
            s->sources = realloc(s->sources,
                    (s->numSources+1)*sizeof(*s->sources));
            s->sources[s->numSources] = f;
            ++s->numSources;
        }
    }

    if(!s->numSources) {
        // It's not going to flow, or call any filter callbacks
        // for there are no sources.
        ERROR("This stream has no sources");
        return -1; // error no sources
    }


    /**********************************************************************
     *            Stage: Check flows for loops, which we can't have
     *********************************************************************/

    for(uint32_t i=0; i<s->numSources; ++i) {
        if(CountFilterPath(s, s->sources[i], 0, s->numConnections) >
                s->numConnections) {
            ERROR("a stream has loops in it");
#ifdef DEBUG
            qsAppDisplayFlowImage(s->app, true);
#endif
            FreeSources(s);
            return -2; // error we have loops
        }
    }

    /**********************************************************************
     *            Stage: Set up filter connections in the filter structs
     *********************************************************************/












    /**********************************************************************
     *            Stage:
     *********************************************************************/





    NOTICE("RUNNING");




    return 0; // success
}

