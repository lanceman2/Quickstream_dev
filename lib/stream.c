#include <stdlib.h>
#include <string.h>

// The public installed user interfaces:
#include "../include/qsapp.h"


// Private interfaces.
#include "./qs.h"
#include "./debug.h"



// TEMPORARY DEBUGGING // TODELETE
#define SPEW(fmt, ... )\
    fprintf(stderr, "%s:line=%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
//#define SPEW(fmt, ... ) /* empty macro */



// The filter that is having start() called.
// There is only one thread when start() is called.
//
struct QsFilter *_qsStartFilter = 0;


// The current filter that is having it's input() called in this thread.
//
__thread struct QsFilter *_qsInputFilter = 0;




struct QsStream *qsAppStreamCreate(struct QsApp *app) {
    
    DASSERT(app, "");

    struct QsStream *s = calloc(1, sizeof(*s));
    ASSERT(s, "calloc(1, %zu) failed", sizeof(*s));

    s->flags = _QS_STREAM_DEFAULTFLAGS;

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

void qsStreamAllowLoops(struct QsStream *s, bool doAllow) {

    DASSERT(s, "");

    if(doAllow)
        s->flags |= _QS_STREAM_ALLOWLOOPS;
    else
        s->flags &= ~_QS_STREAM_ALLOWLOOPS;
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


static inline
void FreeFilterRunResources(struct QsFilter *f) {

    if(f->numOutputs) {
        FreeRingBuffersAndWriters(f);
#ifdef DEBUG
        memset(f->outputs, 0, sizeof(*f->outputs)*f->numOutputs);
#endif
        free(f->outputs);
#ifdef DEBUG
        f->outputs = 0;
#endif
        f->numOutputs = 0;
    }

    f->u.numInputs = 0;
}


static inline
void FreeRunResources(struct QsStream *s) {

    DASSERT(s, "");
    DASSERT(s->app, "");

    for(int32_t i=0; i<s->numConnections; ++i) {
        // These can handle being called more than once per filter.
        FreeFilterRunResources(s->from[i]);
        FreeFilterRunResources(s->to[i]);
    }


    if(s->numSources) {
        // Free the stream sources list
#ifdef DEBUG
        memset(s->sources, 0, sizeof(*s->sources)*s->numSources);
#endif
        free(s->sources);
#ifdef DEBUG
        s->sources = 0;
#endif
        s->numSources = 0;
    }
}


static uint32_t CountFilterPath(struct QsStream *s,
        struct QsFilter *f, uint32_t loopCount, uint32_t maxCount) {

//DSPEW("filter \"%s\" loopCount=%" PRIu32 " maxCount=%"
//          PRIu32, f->name, loopCount, maxCount);

    if(loopCount > maxCount)
        // break the looping.  Stop recursing.  We must have already
        // started looping, because we counted more filters in the path
        // than the number of filters that exist.
        return loopCount;

    ++loopCount;

    uint32_t returnCount = loopCount;


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
                CountFilterPath(s, s->to[i], loopCount, maxCount);
            // this filter feeds the "to" filter.
            if(count > returnCount)
                // We want the largest filter count in all paths.
                returnCount = count;
        }

//DSPEW("filter \"%s\"  returnCount=%" PRIu32, f->name, returnCount);

    return returnCount;
}


int qsStreamStop(struct QsStream *s) {

    DASSERT(s, "");
    DASSERT(s->app, "");

    if(s->numSources == 0)
        // The stream was not in a flow state, and none of the filter
        // callbacks where called.
        return 0;


    DASSERT(s->numSources, "");
    DASSERT(s->sources, "");


    /**********************************************************************
     *            Stage: free extra run data
     *********************************************************************/
    FreeRunResources(s);

    return 0;
}


static
void AllocateFilterOutputsFrom(struct QsStream *s, struct QsFilter *f) {

    // This will work if there are loops in the graph.

    DASSERT(f->numOutputs == 0, "");
    DASSERT(f->u.numInputs == 0, "");

    // count the number of filters that this filter connects to
    uint32_t i;
    for(i=0; i<s->numConnections; ++i) {
        if(s->from[i] == f)
            ++f->numOutputs;
        if(s->to[i] == f)
            ++f->u.numInputs;
    }

    if(f->numOutputs == 0 || f->outputs)
        // There are no outputs or the outputs have been already
        // allocated.
        return;

    // Make the output array
    DASSERT(!f->outputs, "");
    f->outputs = calloc(1, sizeof(*f->outputs)*f->numOutputs);
    ASSERT(f->outputs, "calloc(1,%zu) failed",
            sizeof(*f->outputs)*f->numOutputs);


    for(i=0; s->from[i] != f; ++i);
    // s->from[i] == f

    for(uint32_t j=0; j<f->numOutputs;) {

        f->outputs[j].filter = s->to[i];

        // Set some possibly non-zero default values:
        f->outputs[j].maxReadThreshold = _QS_DEFAULT_maxReadThreshold;
        f->outputs[j].minReadThreshold = _QS_DEFAULT_minReadThreshold;
        f->outputs[j].maxReadSize = _QS_DEFAULT_maxReadSize;
        // All other values in the QsOutput will start at 0.

        if(s->to[i]->numOutputs == 0)
            // recurse.  Allocate children.
            AllocateFilterOutputsFrom(s, s->to[i]);
        ++j;

        if(j == f->numOutputs)
            // This was the last connection from filter f
            break;

        // go to next connection that is from this filter f
        for(++i; s->from[i] != f; ++i);
        DASSERT(s->from[i] == f, "");
    }
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
     *      Stage: lazy cleanup ??
     *********************************************************************/

    //FreeRunResources(s);


    /**********************************************************************
     *      Stage: Find source filters
     *********************************************************************/

    DASSERT(s->numSources == 0,"");

    for(uint32_t i=0; i < s->numConnections; ++i) {

        struct QsFilter *f = s->from[i];
        uint32_t j=0;
        for(; j<s->numConnections; ++j) {
            if(s->to[j] == f)
                // f is not a source
                break;
        }
        if(!(j < s->numConnections) && !f->u.isSource) {
            ++s->numSources;
            f->u.isSource = 1;
        }
    }

 
    if(!s->numSources) {
        // It's not going to flow, or call any filter callbacks
        // because there are no sources.
        ERROR("This stream has no sources");
        // We have nothing to free at this time.
        return -1; // error no sources
    }

    s->sources = malloc(s->numSources*sizeof(*s->sources));
    ASSERT(s->sources, "malloc(,%zu) failed",
            s->numSources*sizeof(*s->sources));

    uint32_t j = 0;
    for(uint32_t i=0; i < s->numConnections; ++i) {
        if(s->from[i]->u.isSource) {
            s->sources[j++] = s->from[i];
            // reset flag so that s->from[i]->u.numInputs == 0
            // and we use it for the number of inputs after here.
            s->from[i]->u.isSource = 0;
        }
    }

    DASSERT(j == s->numSources, "");


    /**********************************************************************
     *      Stage: Check flows for loops, if we can't have them
     *********************************************************************/

    if(!(s->flags && _QS_STREAM_ALLOWLOOPS))
        // In this case, we do not allow loops in the filter graph.
        for(uint32_t i=0; i<s->numSources; ++i) {
            if(CountFilterPath(s, s->sources[i], 0, s->numConnections) >
                    s->numConnections + 1) {
                ERROR("a stream has loops in it consider"
                        " calling: qsStreamAllowLoops()");
//#ifdef DEBUG
                qsAppDisplayFlowImage(s->app, 0, true);
//#endif
                FreeRunResources(s);
                return -2; // error we have loops
            }
        }


    /**********************************************************************
     *      Stage: Set up filter output connections in the filter structs
     *********************************************************************/

    for(uint32_t i=0; i<s->numSources; ++i)
        AllocateFilterOutputsFrom(s, s->sources[i]);


    /**********************************************************************
     *      Stage: call all stream's filter start() if present
     *********************************************************************/

    // By using the app list of filters we do not call any filter start()
    // more than once, (TODO) but is the order in which we call them okay?
    //
    for(struct QsFilter *f = s->app->filters; f; f = f->next)
        if(f->stream == s && f->start) {
            // We mark which filter we are calling the start() for so that
            // if the filter start() calls any filter API function to get
            // resources we know what filter these resources belong to.
            _qsStartFilter = f;
            // Call a filter start() function:
            int ret = f->start(f->u.numInputs, f->numOutputs);
            _qsStartFilter = 0;
            if(ret) {
                // TODO: Should we call filter stop() functions?
                //
                ERROR("filter \"%s\" start()=%d failed", f->name, ret);
                return -3;
            }
        }


    /**********************************************************************
     *     Stage: Allocate flow buffers
     *********************************************************************/

    // Any filters' special buffer requirements should have been gotten
    // from the filters' start() function.  Now we can allocated the
    // memory that is the conveyor belt between filters.  We follow every
    // path in the stream:
    for(uint32_t i=0; i<s->numSources; ++i)
        // It easier to now because the f->outputs are more setup now.
        // If not we setup buffering connectivity defaults.
        AllocateRingBuffers(s->sources[i]);


    /**********************************************************************
     *     Stage: flow
     *********************************************************************/

    NOTICE("RUNNING");

    if(s->numThreads == 0)
        stream_run_0p_0t(s);


    /**********************************************************************
     *     Stage: flush?
     *********************************************************************/




    // We are done running this stream now.


    /**********************************************************************
     *     Stage: call all stream's filter stop() if present
     *********************************************************************/

    for(struct QsFilter *f = s->app->filters; f; f = f->next)
        if(f->stream == s && f->stop)
            f->stop(f->u.numInputs, f->numOutputs);


    /**********************************************************************
     *     Stage: cleanup
     *********************************************************************/

    FreeRunResources(s);



    return 0; // success
}
