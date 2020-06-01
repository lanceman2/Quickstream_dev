#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dlfcn.h>
#include <stdatomic.h>

// The public installed user interfaces:
#include "../include/quickstream/app.h"
#include "../include/quickstream/filter.h"
#include "controllerCallbacks.h"

// Private interfaces.
#include "debug.h"
#include "Dictionary.h"
#include "qs.h"
#include "filterList.h"
#include "stream.h"
#include "parameter.h"



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



// Allocate the array filter->outputs and filter->outputs[].reader, and
// recure to all filters in the stream (s).
//
static bool
AllocateFilterOutputsFrom(struct QsStream *s, struct QsFilter *f,
        bool ret) {

    // This will work if there are loops in the graph.

    DASSERT(f->numOutputs == 0);
    DASSERT(f->numInputs == 0);

    f->mark = false; // Mark the filter output as dealt with.

    DASSERT(f->outputs == 0);

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
            else if(s->connections[i].fromPortNum == QS_NEXTPORT)
                ++f->numOutputs;
            else if(s->connections[i].fromPortNum < f->numOutputs)
                continue;
            else {
                // This is a bad output port number so we just do this
                // default action instead of adding a failure mode, except
                // in DEBUG we'll assert.
                DASSERT(false, "filter \"%s\" has output port number %"
                        PRIu32
                        " out of sequence; setting it to %" PRIu32,
                        s->connections[i].from->name,
                        s->connections[i].fromPortNum,
                        f->numOutputs);
                ERROR("filter \"%s\" has output port number %" PRIu32
                        " out of sequence; setting it to %" PRIu32,
                        s->connections[i].from->name,
                        s->connections[i].fromPortNum,
                        f->numOutputs);

                s->connections[i].fromPortNum = f->numOutputs++;
            }
        }
    }


    if(f->numOutputs == 0)
        // There are no outputs.
        return ret;

    // Make the output array
    ASSERT(f->numOutputs <= _QS_MAX_CHANNELS,
            "%" PRIu32 " > %" PRIu32 " outputs",
            f->numOutputs, _QS_MAX_CHANNELS);

    f->outputs = calloc(f->numOutputs, sizeof(*f->outputs));
    ASSERT(f->outputs, "calloc(%" PRIu32 ",%zu) failed",
            f->numOutputs, sizeof(*f->outputs));

    // Now setup the readers array in each output
    //
    for(uint32_t outputPortNum = 0; outputPortNum<f->numOutputs;
            ++outputPortNum) {

        f->outputs[outputPortNum].maxWrite = QS_DEFAULTMAXWRITE;

        // Count the number of readers for this output port
        // (outputPortNum):
        uint32_t numReaders = 0;
        for(i=0; i<s->numConnections; ++i)
            if(s->connections[i].from == f) {
                if(s->connections[i].fromPortNum == outputPortNum)
                    ++numReaders;
                else if(s->connections[i].fromPortNum == QS_NEXTPORT) {
                    if(numReaders)
                        s->connections[i].fromPortNum = outputPortNum+1;
                    else {
                        ++numReaders;
                        s->connections[i].fromPortNum = outputPortNum;
                    }
                }
            }

        DASSERT(numReaders);
        DASSERT(_QS_MAX_CHANNELS >= numReaders);

        struct QsReader *readers =
            calloc(numReaders, sizeof(*readers));
        ASSERT(readers, "calloc(%" PRIu32 ",%zu) failed",
                numReaders, sizeof(*readers));
        f->outputs[outputPortNum].readers = readers;
        f->outputs[outputPortNum].numReaders = numReaders;

        // Now set the values in reader:
        uint32_t readerIndex = 0;
        for(i=0; i<s->numConnections; ++i)
            if(s->connections[i].from == f &&
                    s->connections[i].fromPortNum == outputPortNum) {
                struct QsReader *reader = readers + readerIndex;
                reader->filter = s->connections[i].to;
                reader->feedFilter = f;
                reader->threshold = QS_DEFAULTTHRESHOLD;
                reader->maxRead = QS_DEFAULTMAXREADPROMISE;

                // We'll set the reader->inputPortNum later in
                // SetupInputPorts(), if inputPortNum is QS_NEXTPORT.
                // We don't now because we are not counting input filters
                // now.
                //
                reader->inputPortNum = s->connections[i].toPortNum;
                ++readerIndex;
            }

        DASSERT(readerIndex == numReaders);
    }


    // Now recurse if we need to.
    //
    for(i=0; i<f->numOutputs; ++i) {
        uint32_t numReaders = f->outputs[i].numReaders;
        for(uint32_t j=0; j<numReaders; ++j) {
            struct QsFilter *filter = f->outputs[i].readers[j].filter;
            DASSERT(filter);
            if(filter->mark)
                ret = AllocateFilterOutputsFrom(s, filter, ret);
        }
    }

    return ret;
}


// ret is a return value passed back from recusing.
//
// This happens after AllocateFilterOutputsFrom() above.
//
static bool
SetupInputPorts(struct QsStream *s, struct QsFilter *f, bool ret) {

    f->mark = false; // filter done

    DASSERT(f->numInputs == 0, "");


#define MARKER  ((uint8_t *)1)

    // For every filter that has output:
    for(uint32_t i=0; i<s->numConnections; ++i) {
        struct QsFilter *outFilter = s->connections[i].from;
        DASSERT(outFilter);
        // Every output
        for(uint32_t j=0; j<outFilter->numOutputs; ++j) {
            struct QsOutput *output = outFilter->outputs + j;
            DASSERT(output);
            DASSERT(output->numReaders);
            DASSERT(output->readers);
            for(uint32_t k=0; k<output->numReaders; ++k) {
                // Every reader
                struct QsReader *reader = output->readers + k;
                if(reader->filter == f && reader->readPtr == 0) {

                    ++f->numInputs;
                    // We borrow this readPtr variable to mark this reader
                    // as counted.
                    reader->readPtr = MARKER;
                }
            }
        }
    }


    DASSERT(f->numInputs <= _QS_MAX_CHANNELS);

    // Now check the input port numbers.

    uint32_t inputPortNums[f->numInputs];
    uint32_t inputPortNum = 0;
    memset(inputPortNums, 0, sizeof(inputPortNums));

    // For every filter that has output:
    for(uint32_t i=0; i<s->numConnections; ++i) {
        struct QsFilter *outFilter = s->connections[i].from;
        // Every output
        for(uint32_t j=0; j<outFilter->numOutputs; ++j) {
            struct QsOutput *output = &outFilter->outputs[j];
            for(uint32_t k=0; k<output->numReaders; ++k) {
                // Every reader
                struct QsReader *reader = output->readers + k;
                // We borrow this readPtr variable again to mark this reader
                // as not counted; undoing the setting of readPtr.
                if(reader->filter == f && reader->readPtr == MARKER) {
                    // Undo the setting of readPtr.
                    reader->readPtr = 0;

                    if(reader->inputPortNum == QS_NEXTPORT)
                        reader->inputPortNum = inputPortNum;
                    if(reader->inputPortNum < f->numInputs)
                        // Mark inputPortNums[] as gotten.
                        inputPortNums[reader->inputPortNum] = 1;
                    //else:  We have a bad input port number.

                    ++inputPortNum;
                }
            }
        }
    }

    DASSERT(inputPortNum == f->numInputs);


    // Check that we have all input port numbers, 0 to N-1, being written
    // to.
    for(uint32_t i=0; i<f->numInputs; ++i)
        if(inputPortNums[i] != 1) {
            ret = true;
            WARN("filter \"%s\" has some bad input port numbers",
                    f->name);
            break; // nope
        }


    if(f->numInputs) {
        // Allocate and set the
        // filter->readers[inputPort] = output feeding reader
        //
        f->readers = calloc(f->numInputs, sizeof(*f->readers));
        ASSERT(f->readers, "calloc(%" PRIu32 ",%zu) failed",
                f->numInputs, sizeof(*f->readers));

        // We got to look at all filter outputs in the stream to find the
        // input readers for this filter, f.
        for(uint32_t i=0; i<s->numConnections; ++i)
            // This i loop may repeat some filters, but that may be better
            // than adding more data structures to the stream struct.
            if(s->connections[i].to == f) {
                struct QsOutput *outputs = s->connections[i].from->outputs;
                uint32_t numOutputs = s->connections[i].from->numOutputs;
                for(uint32_t j=0; j<numOutputs; ++j) {
                    struct QsReader *readers = outputs[j].readers;
                    uint32_t numReaders = outputs[j].numReaders;
                    for(uint32_t k=0; k<numReaders; ++k)
                        if(readers[k].filter == f) {
                            uint32_t inputPortNum = readers[k].inputPortNum;
                            DASSERT(inputPortNum < f->numInputs);
                            // It should only have one reader from one
                            // output, so:
                            if(f->readers[inputPortNum] == 0)
                                // and we set it once here for all inputs
                                // found:
                                f->readers[inputPortNum] = readers + k;
                            else {
                                DASSERT(f->readers[inputPortNum] ==
                                        readers + k);
                                // We have already looked at this filter
                                // and output.  We are using the stream
                                // connection list which can have filters
                                // listed more than once.
                                j = numOutputs; // pop out of j loop
                                break; // pop out of this k loop
                            }
                        }
                }
            }
    }

#ifdef DEBUG
    for(uint32_t i=0; i<f->numInputs; ++i)
        DASSERT(f->readers[i]);
#endif

    // Now recurse to filters that read from this filter if we have not
    // already.
    for(uint32_t i=0; i<f->numOutputs; ++i) {
        struct QsOutput *output = &f->outputs[i];
        for(uint32_t j=0; j<output->numReaders; ++j) {
            // Every reader
            struct QsReader *reader = output->readers + j;
            if(reader->filter->mark)
                // Recurse
                ret = SetupInputPorts(s, reader->filter, ret);
        }
    }

    return ret;
}


static inline
int preStop_callback(struct QsController *c, struct QsStream *s) {

    if(c->preStop) {

        DASSERT(c->mark == 0);
        DASSERT(pthread_getspecific(_qsControllerKey) == 0);
        c->mark = _QS_IN_PRESTOP;
        CHECK(pthread_setspecific(_qsControllerKey, c));

        int ret = 0;

        for(struct QsFilter *f=s->filters; f; f = f->next) {
            ret = c->preStop(s, f, f->numInputs, f->numOutputs);
            if(ret)
                break;
        }

        CHECK(pthread_setspecific(_qsControllerKey, 0));
        c->mark = 0;

        if(ret < 0) {
            ERROR("Controller \"%s\" preStop() returned (%d) error",
                    c->name, ret);
            ASSERT(0, "Write more code here to handle this error case");
        }
    }
    return 0;
}


static inline
int postStop_callback(struct QsController *c, struct QsStream *s) {

    if(c->postStop) {

        DASSERT(c->mark == 0);
        DASSERT(pthread_getspecific(_qsControllerKey) == 0);
        c->mark = _QS_IN_POSTSTOP;
        CHECK(pthread_setspecific(_qsControllerKey, c));

        int ret = 0;

        for(struct QsFilter *f=s->filters; f; f = f->next) {
            ret = c->postStop(s, f, f->numInputs, f->numOutputs);
            if(ret)
                break;
        }

        CHECK(pthread_setspecific(_qsControllerKey, 0));
        c->mark = 0;

        if(ret < 0) {
            ERROR("Controller \"%s\" postStop() returned (%d) error",
                    c->name, ret);
            ASSERT(0, "Write more code here to handle this error case");
        }
    }
    return 0;
}


struct ControllerCallbackRemover {

    // Start and end of a singly linked list of callbacks to remove.
    //
    // Uses (struct Callback)::next to make the list.
    struct  ControllerCallback *start, *end;
};


static void
MarkInputCallback(const char *key, struct  ControllerCallback *cb,
        struct ControllerCallbackRemover *r) {

    if(cb->returnValue) {

        // Add this callback to the list to be removed later.
        if(r->start)
            r->end->next = cb;
        else
            r->start = cb;
        r->end = cb;
        cb->next = 0;

        cb->key = key;
    }
}


int qsStreamStop(struct QsStream *s) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    DASSERT(s);
    DASSERT(s->app);
    DASSERT(!(s->flags & _QS_STREAM_START), "We are starting");
    DASSERT(!(s->flags & _QS_STREAM_STOP), "We are stopping");
    ASSERT(pthread_getspecific(_qsKey) == 0,
            "stream is in the wrong mode to call this now.");
    ASSERT(s->sources != 0,
            "stream is in the wrong mode to call this now.");


    s->flags &= (~_QS_STREAM_LAUNCHED);

    if(!s->sources) {
        // The setup of the stream failed and the user ignored it.
        WARN("The stream is not setup");
        return -1; // failure
    }


    /**********************************************************************
     *      Stage: call all the app's controller preStop()s if present
     *********************************************************************/

    // We call them in reverse load order:
    for(struct QsController *c=s->app->last; c; c=c->prev)
        preStop_callback(c, s);


    /**********************************************************************
     *      Stage: remove all PostInputCallbacks that are marked as
     *             finished.
     *********************************************************************/

    for(struct QsFilter *f = s->filters; f; f = f->next) {

        if(f->postInputCallbacks == 0)
            continue;

        struct ControllerCallbackRemover r;
        r.start = 0;
        r.end = 0;

        qsDictionaryForEach(f->postInputCallbacks,
            (int (*) (const char *key, void *value,
                void *userData)) MarkInputCallback, &r);

        struct ControllerCallback *next;
        for(struct ControllerCallback *cb=r.start; cb; cb = next) {
            next = cb->next;
            DSPEW("Removing %s:%s PostInput callback", f->name, cb->key);
            qsDictionaryRemove(f->postInputCallbacks, cb->key);
        }
    }


    /**********************************************************************
     *     Stage: call all stream's filter stop() if present
     *********************************************************************/

    for(struct QsFilter *f = s->filters; f; f = f->next)
        if(f->stream == s && f->stop) {
            CHECK(pthread_setspecific(_qsKey, f));
            f->mark = _QS_IN_STOP;
            s->flags |= _QS_STREAM_STOP;
            f->stop(f->numInputs, f->numOutputs);
            s->flags &= ~_QS_STREAM_STOP;
            f->mark = 0;
            CHECK(pthread_setspecific(_qsKey, 0));
        }


    /**********************************************************************
     *      Stage: call all the app's controller postStop()s if present
     *********************************************************************/

    // We call them in reverse load order:
    for(struct QsController *c=s->app->last; c; c=c->prev)
        postStop_callback(c, s);


    /**********************************************************************
     *     Stage: remove all the parameter get callbacks that we can.
     *********************************************************************/

    for(struct QsFilter *f = s->filters; f; f = f->next)
        _qsParameterRemoveCallbacksForRestart(f);


    /**********************************************************************
     *     Stage: cleanup
     *********************************************************************/

    FreeRunResources(s);

    s->flow = 0;

    return 0; // success
}


static
int preStart_callback(struct QsController *c, struct QsStream *s) {

    if(c->preStart) {

        DASSERT(c->mark == 0);
        DASSERT(pthread_getspecific(_qsControllerKey) == 0);
        c->mark = _QS_IN_PRESTART;
        CHECK(pthread_setspecific(_qsControllerKey, c));

        int ret = 0;

        for(struct QsFilter *f=s->filters; f; f = f->next) {
            ret = c->preStart(s, f, f->numInputs, f->numOutputs);
            if(ret)
                break;
        }

        CHECK(pthread_setspecific(_qsControllerKey, 0));
        c->mark = 0;

        if(ret < 0) {
            ERROR("Controller \"%s\" preStart() returned (%d) error",
                    c->name, ret);
            ASSERT(0, "Write more code here to handle this error case");
        }
    }
    return 0;
}


static
int postStart_callback(struct QsController *c, struct QsStream *s) {

    if(c->postStart) {

        DASSERT(c->mark == 0);
        DASSERT(pthread_getspecific(_qsControllerKey) == 0);
        c->mark = _QS_IN_POSTSTART;
        CHECK(pthread_setspecific(_qsControllerKey, c));

        int ret = 0;

        for(struct QsFilter *f=s->filters; f; f = f->next) {
            ret = c->postStart(s, f, f->numInputs, f->numOutputs);
            if(ret)
                break;
        }

        CHECK(pthread_setspecific(_qsControllerKey, 0));
        c->mark = 0;

        if(ret < 0) {
            ERROR("Controller \"%s\" postStart() returned (%d) error",
                    c->name, ret);
            ASSERT(0, "Write more code here to handle this error case");
        }
    }
    return 0;
}



int qsStreamReady(struct QsStream *s) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    DASSERT(s);
    DASSERT(s->app);
    DASSERT(!(s->flags & _QS_STREAM_START), "We are starting");
    DASSERT(!(s->flags & _QS_STREAM_STOP), "We are stopping");
    ASSERT(pthread_getspecific(_qsKey) == 0,
            "The stream is in the wrong mode to call this now.");
    ASSERT(s->sources == 0,
            "The stream is in the wrong mode to call this now.");


    ///////////////////////////////////////////////////////////////////////
    //                                                                   //
    //  This has a few stages in which we go through the lists, check    //
    //  things out and set things up.                                    //
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

    DASSERT(s->numSources == 0);

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

    DASSERT(j == s->numSources);


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
        if(AllocateFilterOutputsFrom(s, s->sources[i], false)) {
//#ifdef DEBUG
            qsAppDisplayFlowImage(s->app, 0, true);
//#endif
            FreeRunResources(s);
            return -3; // error we have loops
        }


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
     *      Stage: call all the app's controller preStart()s if present
     *********************************************************************/

    // We call them in load order:
    for(struct QsController *c=s->app->first; c; c=c->next)
        preStart_callback(c, s);


    /**********************************************************************
     *      Stage: call all stream's filter start() if present
     *********************************************************************/

    // By using the app list of filters we do not call any filter start()
    // more than once, (TODO) but is the order in which we call them okay?
    //
    for(struct QsFilter *f = s->filters; f; f = f->next) {
        if(f->stream == s) {
            if(f->start) {
                // We mark which filter we are calling the start() for so
                // that if the filter start() calls any filter API
                // function to get resources we know what filter these
                // resources belong to.
                CHECK(pthread_setspecific(_qsKey, f));
                f->mark = _QS_IN_START;
                s->flags |= _QS_STREAM_START;
                // Call a filter start() function:
                int ret = f->start(f->numInputs, f->numOutputs);
                s->flags &= ~_QS_STREAM_START;
                f->mark = 0;
                CHECK(pthread_setspecific(_qsKey, 0));

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
     *     Stage: allocate output buffer (QsBuffer) structures
     *********************************************************************/

    // Any filters' special buffer requirements should have been gotten
    // from the filter's start() function.  Now we allocate any output
    // buffers that have not been explicitly allocated from the filter
    // start() calling qsCreateOutputBuffer().
    //
    StreamSetFilterMarks(s, true);
    for(uint32_t i=0; i<s->numSources; ++i)
        AllocateBuffer(s->sources[i]);


    /**********************************************************************
     *     Stage: mmap() ring buffers with memory mappings
     *********************************************************************/

    // There may be some calculations needed from the buffer structure for
    // the memory mapping, so we do this in a different loop.
    //
    // This calculates the memory mapping sizes from the filter promised
    // write and read sizes and calls mmap().
    //
    StreamSetFilterMarks(s, true);
    for(uint32_t i=0; i<s->numSources; ++i)
        MapRingBuffers(s->sources[i]);


    /**********************************************************************
     *      Stage: call all the app's controller postStart()s if present
     *********************************************************************/

    // We call them in load order:
    for(struct QsController *c=s->app->first; c; c=c->next)
        postStart_callback(c, s);


    return 0; // success
}
