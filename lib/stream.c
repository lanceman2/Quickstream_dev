#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// The public installed user interfaces:
#include "../include/qsapp.h"
#include "../include/qsfilter.h"

// Private interfaces.
#include "./qs.h"
#include "./debug.h"



// TEMPORARY DEBUGGING // TODELETE
#define SPEW(fmt, ... )\
    fprintf(stderr, "%s:line=%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
//#define SPEW(fmt, ... ) /* empty macro */



// The filter that is having start(), stop(), construct(), or destroy()
// called.  There is only one thread when these functions are called.
//
struct QsFilter *_qsCurrentFilter = 0;



struct QsStream *qsAppStreamCreate(struct QsApp *app,
        uint32_t maxThreads) {

    DASSERT(app, "");

    struct QsStream *s = calloc(1, sizeof(*s));
    ASSERT(s, "calloc(1, %zu) failed", sizeof(*s));
    s->maxThreads = _QS_STREAM_DEFAULTMAXTHTREADS;
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
    s->maxThreads = maxThreads;
    s->flags = _QS_STREAM_DEFAULTFLAGS;
    if(maxThreads) {
        // If we use more than on thread we'll need to be able to control
        // them with a condition variable and mutex.  We also will need to
        // control access to some data using the mutex.
        //
        ASSERT(pthread_cond_init(&s->cond, 0) == 0, "");
        ASSERT(pthread_mutex_init(&s->mutex, 0) == 0, "");
    }

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
    DASSERT(s->numConnections || s->connections == 0, "");

    if(s->sources) {
        DASSERT(s->numSources, "");
#ifdef DEBUG
        memset(s->sources, 0, sizeof(*s->sources)*s->numSources);
#endif
        free(s->sources);
    }

    if(s->numConnections) {

        DASSERT(s->connections, "");
#ifdef DEBUG
        memset(s->connections, 0, sizeof(*s->connections)*s->numConnections);
#endif
        free(s->connections);
        s->connections = 0;
        s->numConnections = 0;
    }
    if(s->maxThreads) {
        ASSERT(pthread_mutex_destroy(&s->mutex) == 0, "");
        ASSERT(pthread_cond_destroy(&s->cond) == 0, "");
    }
#ifdef DEBUG
    memset(s, 0, sizeof(*s));
#endif
    free(s);
}


void qsStreamDestroy(struct QsStream *s) {

    DASSERT(s, "");
    DASSERT(s->app, "");
    DASSERT(s->numConnections || s->connections == 0, "");

    // Remove this stream from all the filters in the listed connections.
    for(uint32_t i=0; i<s->numConnections; ++i) {
        DASSERT(s->connections[i].from, "");
        DASSERT(s->connections[i].to, "");
        DASSERT(s->connections[i].from->stream == s ||
                s->connections[i].from->stream == 0, "");
        DASSERT(s->connections[i].to->stream == s ||
                s->connections[i].to->stream == 0, "");

        s->connections[i].from->stream = 0;
        s->connections[i].to->stream = 0;
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
}



void qsStreamConnectFilters(struct QsStream *s,
        struct QsFilter *from, struct QsFilter *to,
        uint32_t fromPortNum, uint32_t toPortNum) {

    DASSERT(s, "");
    DASSERT(s->app, "");
    ASSERT(from != to, "a filter cannot connect to itself");
    DASSERT(from, "");
    DASSERT(to, "");
    DASSERT(to->app, "");
    DASSERT(from->app == s->app, "wrong app");
    DASSERT(to->app == s->app, "wrong app");
    DASSERT(to->stream == s || !to->stream,
            "filter cannot be part of another stream");
    DASSERT(from->stream == s || !from->stream,
            "filter cannot be part of another stream");

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

    DASSERT(s, "");
    DASSERT(i < -1, "");
    DASSERT(s->numConnections > i, "");

    // This is how many connections there will be after this function
    --s->numConnections;

    struct QsFilter *from = s->connections[i].from;
    struct QsFilter *to = s->connections[i].to;

    DASSERT(from, "");
    DASSERT(to, "");
    DASSERT(from->stream == s, "");
    DASSERT(to->stream == s, "");
    // Outputs should not be allocated yet.
    DASSERT(from->outputs == 0, "");
    DASSERT(to->outputs == 0, "");
    DASSERT(from->numOutputs == 0, "");
    DASSERT(to->numOutputs == 0, "");

    if(s->numConnections == 0) {

        // Singular case.  We have no connections anymore.
        //
        from->stream = 0;
        to->stream = 0;
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

    // If this "from" filter is not in this stream mark it as such.
    for(i=0; i<s->numConnections; ++i)
        if(s->connections[i].from == from || s->connections[i].to == from)
            break;
    if(i==s->numConnections)
        // This "from" filter has no stream associated with it:
        from->stream = 0;

    // If the "to" filter is not in this stream mark it as such.
    for(i=0; i<s->numConnections; ++i)
        if(s->connections[i].from == to || s->connections[i].to == to)
            break;
    if(i==s->numConnections)
        // This "to" filter has no stream associated with it:
        to->stream = 0;
}



int qsStreamRemoveFilter(struct QsStream *s, struct QsFilter *f) {

    DASSERT(s, "");
    DASSERT(f, "");
    DASSERT(f->stream == s, "");
    DASSERT(s->numConnections || s->connections == 0, "");

    bool gotOne = false;

    for(uint32_t i=0; i<s->numConnections;) {
        DASSERT(s->connections, "");
        DASSERT(s->connections[i].to, "");
        DASSERT(s->connections[i].from, "");
        DASSERT(s->connections[i].to != s->connections[i].from, "");
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



// Note this is call FreeFilterRunResources; it does not free up all
// filter resources, just some things that could change between stop() and
// start().
//
static inline
void FreeFilterRunResources(struct QsFilter *f) {

    if(f->numOutputs) {
        DASSERT(f->outputs, "");

        UnmapRingBuffers(f);
        FreeBuffers(f);

        // For every output in this filter we free the readers, and
        // mutex and cond, if they were used.
        for(uint32_t i=f->numOutputs-1; i!=-1 --i) {
            struct QsOutput *output = &f->outputs[i];
            DASSERT(output->numReaders, "");
            DASSERT(output->readers, "");
            DASSERT((output->mutex && output->cond) ||
                    (!output->mutex || !output->cond),"");
#ifdef DEBUG
            memset(output->readers, 0,
                    sizeof(*output->readers)*output->numReaders);
#endif

            if(output->mutex) {
                // We had multi-threaded access to this buffer.
                ASSERT(pthread_mutex_destroy(output->mutex) == 0, "");
                ASSERT(pthread_cond_destroy(output->cond) == 0, "");
#ifdef DEBUG
                memset(output->mutex, 0, sizeof(*output->mutex));
                memset(output->cond, 0, sizeof(*output->cond));
#endif
                free(output->mutex);
                free(output->cond);
            }

            free(output->readers);
        }

#ifdef DEBUG
        memset(f->outputs, 0, sizeof(*f->outputs)*f->numOutputs);
#endif
        free(f->outputs);
#ifdef DEBUG
        f->outputs = 0;
#endif
        f->numOutputs = 0;
        f->isSource = false;
    }

    f->numInputs = 0;
    f->numThreads = 0;
    f->nextThreadNum = 0;
}



// Note this is call FreerRunResources; it does not free up all stream
// resources, just some things that could change between stop() and
// start().
//
static inline
void FreeRunResources(struct QsStream *s) {

    DASSERT(s, "");
    DASSERT(s->app, "");

    for(int32_t i=0; i<s->numConnections; ++i) {
        // These can handle being called more than once per filter.
        FreeFilterRunResources(s->connections[i].from);
        FreeFilterRunResources(s->connections[i].to);
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
        if(s->connections[i].from == f) {
            uint32_t count =
                // recurse
                CountFilterPath(s, s->connections[i].to,
                        loopCount, maxCount);
            // this filter feeds the "to" filter.
            if(count > returnCount)
                // We want the largest filter count in all paths.
                returnCount = count;
        }

//DSPEW("filter \"%s\"  returnCount=%" PRIu32, f->name, returnCount);

    return returnCount;
}



// Allocate the array filter->outputs, and recure to all filters in the
// stream (s).
//
static
void AllocateFilterOutputsFrom(struct QsStream *s, struct QsFilter *f) {

    // This will work if there are loops in the graph.

    DASSERT(f->numOutputs == 0, "");
    DASSERT(f->numInputs == 0, "");

    f->mark = false; // Mark the filter output as dealt with.

    DASSERT(f->outputs == 0, "");

    // Count the number of filters that this filter connects to with a
    // different output port number.
    uint32_t i;
    for(i=0; i<s->numConnections; ++i) {
        if(s->connections[i].from == f) {
            // There can be many connections to one output port.  We
            // require that output port numbers be in a sequence, but some
            // output ports feed more than one filter.  We are not dealing
            // with input port numbers at this time.
            if(s->connections[i].fromPortNum == f->numOutputs)
                ++f->numOutputs;
            else if(s->connections[i].fromPortNum + 1 == f->numOutputs)
                continue;
            else if(s->connections[i].fromPortNum == QS_NEXTPORT)
                ++f->numOutputs;
            else {
                char *s = "filter \"%s\" has output port number % "
                        PRIu32 " out of sequence; setting it to %"
                        PRIu32;
                // This is a bad output port number so we just do this
                // default action instead of adding a failure mode, except
                // in DEBUG we'll assert.
                WARN(s, s->connections[i].fromPortNum,
                        f->numOutputs);
                DASSERT(false, s, s->connections[i].fromPortNum,
                        f->numOutputs);
                s->connections[i].fromPortNum = f->numOutputs++;
            }
        }
    }

    if(f->numOutputs == 0) {
        // There are no outputs.
        return;

    // Make the output array
    ASSERT(f->numOutputs <= QS_MAX_CHANNELS,
            "%" PRIu32 " > %" PRIu32 " outputs",
            f->numOutputs, QS_MAX_CHANNELS);

    f->outputs = calloc(f->numOutputs, sizeof(*f->outputs));
    ASSERT(f->outputs, "calloc(%" PRIu32 ",%zu) failed",
            f->numOutputs, sizeof(*f->outputs));


    // Now setup the readers array in each output
    //
    for(uint32_t outputPortNum = 0; outputPortNum<f->numOutputs;) {

        // Count the number of readers for this output port
        // (outputPortNum):
        uint32_t numReaders = 0;
        for(i=0; i<s->numConnections; ++i)
            if(s->connections[i].from == f &&
                    s->connection[i].fromPortNum == outputPortNum)
                ++numReaders;

        DASSERT(numReaders, "filter \"%s\" output port %"
                PRIu32 " has no readers", f->name, outputPortNum);

        struct QsReader *readers =
            calloc(numReaders, sizeof(*readers));
        ASSERT(readers, "calloc(%" PRIu32 ",%zu) failed",
                numReaders, sizeof(*readers));

        // Now set the values in reader:
        uint32_t readerIndex = 0;
        for(i=0; i<s->numConnections; ++i)
            if(s->connections[i].from == f &&
                    s->connections[i].fromPortNum == outputPortNum) {
                reader[readerIndex].filter = s->connections[i].to;
                reader[readerIndex].thresholdLength = _QS_DEFAULT_THRESHOLD;
                reader[readerIndex].inputPortNum = s->connections[i].toPortNum;
                ++readerIndex;
            }
        DASSERT(readerIndex == numReaders, "");
        DASSERT(QS_MAX_CHANNELS >= numReaders, "");
    }

    // Now recurse if we need to.
    //
    for(i=0; i<f->numOutputs; ++i) {
        uint32_t numReaders = f->outputs[j].numReaders;
        for(uint32_t j=0; j<numReaders; ++j) {
            struct QsFilter *f = f->outputs[j].readers[j].filter;
            DASSERT(f, "");
            if(f->marked)
                AllocateFilterOutputsFrom(s, f);
        }
    }

}


// ret is a return value passed back from recusing.
static bool
SetupInputPorts(struct QsStream *s, struct QsFilter *f, bool ret) {

    f->marked = false; // filter done

    DASSERT(f->numInputs == 0, "");

    // For every filter that has output:
    for(uint32_t i=0; i<s->numConnections; ++i) {
        struct QsFilter *outFilter = s->from[i];
        DASSERT(outFilter, "");
        // Every output
        for(uint32_t j=0; j<outFilter->numOutputs; ++j) {
            struct QsOutput *output = &outFilter->outputs[j];
            DASSERT(output, "");
            DASSERT(output->numReaders, "");
            DASSERT(output->readers, "");
            for(uint32_t k=0; k<output->numReaders; ++k) {
                // Every reader
                struct QsReader *reader = &output->readers[k];
                if(reader->filter == f)
                    ++f->numInputs;
            }
        }
    }

    DASSERT(f->numInputs <= QS_MAX_CHANNELS, "");

    // Now check the input port numbers.

    uint32_t inputPortNums[f->numInputs];
    uint32_t inputPortNum = 0;
    memset(inputPortNums, 0, sizeof(inputPortNums));

    // For every filter that has output:
    for(uint32_t i=0; i<s->numConnections; ++i) {
        struct QsFilter *outFilter = s->from[i];
        // Every output
        for(uint32_t j=0; j<outFilter->numOutputs; ++j) {
            struct QsOutput *output = &outFilter->outputs[j];
            for(uint32_t k=0; k<output->numReaders; ++k) {
                // Every reader
                struct QsReader *reader = &output->readers[k];
                if(reader->filter == f) {
                    if(reader->inputPortNum == QS_NEXTPORT)
                        reader->inputPortNum = inputPortNum;
                    if(reader->inputPortNum < f->numInputs)
                        inputPortNums[reader->inputPortNum] = 1;
                    //else
                    //  We have a bad input port number.
                    ++inputPortNum;
                }
            }
        }
    }

    // Check that we have all input port numbers being written to.
    for(uint32_t i=0; i<f->numInputs; ++i)
        if(inputPortNums[i] != 1) {
            ret = true;
            WARN("filter \"%s\" has some bad output port numbers", f);
            break; // nope
        }

    
    // Now recure to filters that read from this filter if we have not
    // already.
    for(uint32_t i=0; i<f->numOutputs; ++i) {
        struct QsOutput *output = &outFilter->outputs[i];
        for(uint32_t j=0; j<output->numReaders; ++j) {
            // Every reader
            struct QsReader *reader = &output->readers[j];
            if(reader->filter->marked)
                ret = SetupInputPorts(s, reader->filter, ret);
        }
    }

    return ret;
}


// Do one source feed loop.
uint32_t qsStreamFlow(struct QsStream *s) {

    DASSERT(s, "");
    DASSERT(s->app, "");
    ASSERT(s->sources, "qsStreamReady() and qsStreamLaunch() "
            "must be called before this");
    ASSERT(s->flags & _QS_STREAM_LAUNCHED,
            "Stream has not been launched yet");

    DASSERT(s->flow, "");

    return s->flow(s);
}


int qsStreamLaunch(struct QsStream *s) {

    // Make other threads and processes.  Put the thread and processes
    // in a blocking call waiting for the flow.

    DASSERT(s, "");
    DASSERT(s->app, "");
    ASSERT(s->sources, "qsStreamReady() must be successfully"
            " called before this");
    ASSERT(!(s->flags & _QS_STREAM_LAUNCHED),
            "Stream has been launched already");

    // TODO: for the single thread case this does nothing.

    s->flags |= _QS_STREAM_LAUNCHED;


    return 0;
}


int qsStreamStop(struct QsStream *s) {

    DASSERT(s->sources, "");

    s->flags &= (~_QS_STREAM_LAUNCHED);

    if(!s->sources) {
        // The setup of the stream failed and the user ignored it.
        WARN("The stream is not setup");
        return -1;
    }


    /**********************************************************************
     *     Stage: call all stream's filter stop() if present
     *********************************************************************/

    for(struct QsFilter *f = s->app->filters; f; f = f->next)
        if(f->stream == s && f->stop) {
            _qsCurrentFilter = f;
            f->stop(f->numInputs, f->numOutputs);
            _qsCurrentFilter = 0;
        }

    /**********************************************************************
     *     Stage: cleanup
     *********************************************************************/

    FreeRunResources(s);

    s->flow = 0;


    return 0; // success ??
}



int qsStreamReady(struct QsStream *s) {

    DASSERT(s, "");
    DASSERT(s->app, "");

    ///////////////////////////////////////////////////////////////////////
    //                                                                   //
    //  This has a few stages in which we go through the lists, check    //
    //  things out, set things up.                                       //
    //                                                                   // 
    ///////////////////////////////////////////////////////////////////////

    // Failure error return values start at -1 and go down from there;
    // like: -1, -2, -3, -4, ...

    /**********************************************************************
     *      Stage: lazy cleanup from last run??
     *********************************************************************/

    //FreeRunResources(s);


    /**********************************************************************
     *      Stage: Find source filters
     *********************************************************************/

    DASSERT(s->numSources == 0,"");

    for(uint32_t i=0; i < s->numConnections; ++i) {

        struct QsFilter *f = s->connections[i].from;
        uint32_t j=0;
        for(; j<s->numConnections; ++j) {
            if(s->connections[j].to == f)
                // f is not a source
                break;
        }
        if(!(j < s->numConnections) && !f->isSource) {
            // Count the sources.
            ++s->numSources;
            f->isSource = true;
        }
    }

 
    if(!s->numSources) {
        // It's not going to flow, or call any filter callbacks
        // because there are no sources.
        ERROR("This stream has no sources");
        // We have nothing to free at this time so we just return with
        // error.
        return -1; // error no sources
    }

    s->sources = calloc(1, s->numSources*sizeof(*s->sources));
    ASSERT(s->sources, "calloc(1,%zu) failed", 
            s->numSources*sizeof(*s->sources));

    // Get a pointer to the source filters.
    uint32_t j = 0;
    for(uint32_t i=0; i < s->numConnections; ++i)
        if(s->connections[i].from->isSource) {
            uint32_t k=0;
            for(;k<j; ++k)
                if(s->sources[k] == s->connections[i].from)
                    break;
            if(k == j)
                // s->connections[i].from was not in the s->sources[] so
                // add it.
                s->sources[j++] = s->connections[i].from;
        }

    DASSERT(j == s->numSources, "");


    /**********************************************************************
     *      Stage: Check flows for loops, if we can't have them
     *********************************************************************/

    if(!(s->flags && _QS_STREAM_ALLOWLOOPS))
        // In this case, we do not allow loops in the filter graph.
        for(uint32_t i=0; i<s->numSources; ++i)
            if(CountFilterPath(s, s->sources[i], 0, s->numConnections) >
                    s->numConnections + 1) {
                ERROR("stream has loops in it consider"
                        " calling: qsStreamAllowLoops()");
//#ifdef DEBUG
                qsAppDisplayFlowImage(s->app, QSPrintOutline, true);
//#endif
                FreeRunResources(s);
                return -2; // error we have loops
            }


    /**********************************************************************
     *      Stage: Set up filter output and output::readers
     *********************************************************************/

    StreamSetFilterMarks(s, true);
    for(uint32_t i=0; i<s->numSources; ++i)
        AllocateFilterOutputsFrom(s, s->sources[i]);


    /**********************************************************************
     *      Stage: Set up filter input ports
     *********************************************************************/

    StreamSetFilterMarks(s, true);
    for(uint32_t i=0; i<s->numSources; ++i)
        if(SetupInputPorts(s, s->sources[i], false)) {
            ERROR("stream has bad input port numbers");
//#ifdef DEBUG
            qsAppDisplayFlowImage(s->app, QSPrintDebug, true);
//#endif
            FreeRunResources(s);
            return -4; // error we have loops
        }


    /**********************************************************************
     *      Stage: call all stream's filter start() if present
     *********************************************************************/

    // By using the app list of filters we do not call any filter start()
    // more than once, (TODO) but is the order in which we call them okay?
    //
    for(struct QsFilter *f = s->app->filters; f; f = f->next) {
        if(f->stream == s) {
            if(f->start) {
                // We mark which filter we are calling the start() for so that
                // if the filter start() calls any filter API function to get
                // resources we know what filter these resources belong to.
                _qsCurrentFilter = f;
                // Call a filter start() function:
                int ret = f->start(f->numInputs, f->numOutputs);
                _qsCurrentFilter = 0;
                if(ret) {
                    // TODO: Should we call filter stop() functions?
                    //
                    ERROR("filter \"%s\" start()=%d failed", f->name, ret);
                    return -3; // We're screwed.
                }
            }
        }
    }


    /**********************************************************************
     *     Stage: allocate default output buffers
     *********************************************************************/

    // Any filters' special buffer requirements should have been gotten
    // from the filters' start() function.  Now we allocate any output
    // buffers that have not been explicitly allocated from the filter
    // start() calling qsBufferCreate().
    //
    StreamSetFilterMarks(s, false);
    for(uint32_t i=0; i<s->numSources; ++i)
        AllocateBuffer(s->sources[i]);


    /**********************************************************************
     *     Stage: mmap() ring buffers to memory
     *********************************************************************/

    for(uint32_t i=0; i<s->numSources; ++i)
        MapRingBuffers(s->sources[i]);


    /**********************************************************************
     *     Stage: set the flow function
     *********************************************************************/

    // TODO: vary this:
    //
    s->flow = singleThreadFlow;




    return 0; // success
}
