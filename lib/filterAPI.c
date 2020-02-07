#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <pthread.h>

#include "./debug.h"
#include "./qs.h"
#include "./flowJobLists.h"
#include "../include/quickstream/filter.h"


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// In this file is the implementation of most of C functions in the
// quickstream filter module writers API (application programming
// interface).
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


// GetJob() is boiler plate code
// (https://en.wikipedia.org/wiki/Boilerplate_code) that is at the top of
// the API functions that can be called from the filter's input()
// function.
static inline
struct QsJob *GetJob(void) {

    DASSERT(_qsMainThread != pthread_self(), "Is main thread");
    struct QsJob *j = pthread_getspecific(_qsKey);
    // If job, j, was not in thread specific data than this could
    // be due to a user calling this function while not in a filter module
    // input() call.
    ASSERT(j, "Not from code in a filter module input() function");
    // Double check with a magic number if DEBUG
    DASSERT(j->magic == _QS_IS_JOB);

#ifdef DEBUG
    struct QsFilter *f = j->filter;
    DASSERT(f);
    struct QsStream *s = f->stream;
    DASSERT(s);
    DASSERT(!(s->flags & _QS_STREAM_START), "Stream is starting");
    DASSERT(!(s->flags & _QS_STREAM_STOP), "Stream is stopping");
    DASSERT(f->mark == 0, "This finished filter \"%s\" "
            "should not be calling input()", f->name);
#endif

    return j;
}


// This function could be just a very short 1 or 2 line function if not
// for the debugging and error checking.
//
// TODO: consider the multi-threaded filter case.
void qsOutput(uint32_t outputPortNum, const size_t len) {

    struct QsJob *j = GetJob();
    struct QsFilter *f = j->filter;

    DASSERT(f->numOutputs, "Filter \"%s\" has no outputs", f->name);

    // This would be a user error.
    ASSERT(outputPortNum < f->numOutputs,
            "Filter \"%s\", bad output port number",
            f->name);

    // TODO:
    ASSERT(f->maxThreads == 1,
            "Filter \"%s\", multi-threaded filter"
            " case is not written yet", f->name);


    struct QsOutput *output = f->outputs + outputPortNum;
    DASSERT(len <= output->maxWrite);
    DASSERT(output->readers);
    DASSERT(output->numReaders);


    // This is all this function needed to do.
    j->outputLens[outputPortNum] += len;

    // Check for this user error:
    ASSERT(j->outputLens[outputPortNum] <= output->maxWrite,
                "Filter \"%s\" writing %zu which is greater"
                " than the %zu promised",
                f->name, j->outputLens[outputPortNum],
                output->maxWrite);
}


// This function could be just a very short 1 to 3 line function if not
// for the debugging and error checking.
//
// This function must be thread-safe and restraint.
//
// TODO: consider the multi-threaded filter case.
void *qsGetOutputBuffer(uint32_t outputPortNum,
        size_t maxLen, size_t minLen) {

    struct QsJob *j = GetJob();
    struct QsFilter *f = j->filter;

    // TODO:
    ASSERT(f->maxThreads == 1,
            "Filter \"%s\", multi-threaded filter"
            " case is not written yet", f->name);

    // These two ASSERT()s are filter API user errors.
    ASSERT(maxLen);
    ASSERT(maxLen >= minLen);

    DASSERT(f->numOutputs);
    DASSERT(f->outputs);
    ASSERT(f->numOutputs > outputPortNum);

    // TODO:  The multi-threaded case will use the minLen arg.
    ASSERT(f->maxThreads == 1,
            "Filter \"%s\", multi-threaded filter"
            " case is not written yet", f->name);


    struct QsOutput *output = f->outputs + outputPortNum;
    DASSERT(output->readers);
    DASSERT(output->numReaders);

    // Check for this user error
    ASSERT(output->maxWrite >= maxLen);

    return output->writePtr;
}


// TODO: consider the multi-threaded filter case.
void qsAdvanceInput(uint32_t inputPortNum, size_t len) {

    struct QsJob *j = GetJob();
    struct QsFilter *f = j->filter;

    // TODO:
    ASSERT(f->maxThreads == 1,
            "Filter \"%s\", multi-threaded filter"
            " case is not written yet", f->name);

    DASSERT(inputPortNum < f->numInputs);

    j->advanceLens[inputPortNum] += len;

    DASSERT(f->readers);

    // Check if the buffer is being over-read.  If the filter really read
    // this much data than it will have read past the write pointer.
    ASSERT(j->advanceLens[inputPortNum] <=
            f->readers[inputPortNum]->readLength,
            "Filter \"%s\" on input port %" PRIu32
            " tried to read to much %zu > %zu available",
            f->name,
            inputPortNum, j->advanceLens[inputPortNum],
            f->readers[inputPortNum]->readLength);
}


void qsSetInputThreshold(uint32_t inputPortNum, size_t len) {

    // We only call this in the main thread in start().
    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    struct QsFilter *f = pthread_getspecific(_qsKey);
    DASSERT(f);
    // User error checked via magic number _QS_IN_START.
    ASSERT(f->mark == _QS_IN_START, "Not in filter start()");
    struct QsStream *s = f->stream;
    DASSERT(s);
    ASSERT(s->flags & _QS_STREAM_START, "Stream is not starting");
    DASSERT(f->numInputs, "Filter \"%s\" has no inputs", f->name);
    DASSERT(!(s->flags & _QS_STREAM_STOP), "Stream is stopping");
    // This would be a user error.
    ASSERT(inputPortNum < f->numInputs);
    DASSERT(f->readers);

    if(len > f->readers[inputPortNum]->maxRead)
        // maxRead must be <= threshold
        f->readers[inputPortNum]->maxRead = len;

    f->readers[inputPortNum]->threshold = len;

    ASSERT(0, "Write this code");
}



void qsSetInputReadPromise(uint32_t inputPortNum, size_t len) {

    // We only call this in the main thread in start().
    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    struct QsFilter *f = pthread_getspecific(_qsKey);
    DASSERT(f);
    // User error checked via magic number _QS_IN_START.
    ASSERT(f->mark == _QS_IN_START, "Not in filter start()");
    struct QsStream *s = f->stream;
    DASSERT(s);
    ASSERT(s->flags & _QS_STREAM_START, "Stream is not starting");
    DASSERT(f->numInputs, "Filter \"%s\" has no inputs", f->name);
    DASSERT(!(s->flags & _QS_STREAM_STOP), "Stream is stopping");
    // This would be a user error.
    ASSERT(inputPortNum < f->numInputs);
    DASSERT(f->readers);

    if(f->readers[inputPortNum]->threshold > len)
        // maxRead must be <= threshold
        f->readers[inputPortNum]->threshold = len;

    f->readers[inputPortNum]->maxRead = len;
}


// Here we just allocate the output buffer structure.  Later we will
// mmap() the ring buffers, after all the filter start()s are called.
//
// This will not be a "pass-through" buffer.
void qsCreateOutputBuffer(uint32_t outputPortNum, size_t maxWriteLen) {

    // We only call this in the main thread in start().
    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    struct QsFilter *f = pthread_getspecific(_qsKey);
    DASSERT(f);
    ASSERT(f->mark == _QS_IN_START, "Not in filter start()");
    struct QsStream *s = f->stream;
    DASSERT(s);
    ASSERT(s->flags & _QS_STREAM_START, "Stream is not starting");
    DASSERT(!(s->flags & _QS_STREAM_STOP), "Stream is stopping");
    // This would be a user error.
    ASSERT(outputPortNum < f->numOutputs);
    DASSERT(f->outputs);

    f->outputs[outputPortNum].maxWrite = maxWriteLen;

    // We allocate the resources needed for this later.
}


static inline
struct QsOutput *FindFeedOutput(struct QsFilter *feed, struct QsFilter *fed,
        uint32_t fedInPort) {

    for(uint32_t i=feed->numOutputs-1; i!=-1; --i) {
        struct QsOutput *output = feed->outputs + i;
        for(uint32_t j=output->numReaders-1; j!=-1; --j)
            // See if this output has this fed filter reading it on
            // fed's input port fedInPort.
            if(output->readers[j].filter == fed &&
                    output->readers[j].inputPortNum == fedInPort)
                return output;
    }
    return 0;
}


int
qsCreatePassThroughBuffer(uint32_t inPortNum, uint32_t outPortNum,
        size_t maxWriteLen) {
    // We only call this in the main thread in start().
    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    struct QsFilter *f = pthread_getspecific(_qsKey);
    DASSERT(f);
    ASSERT(f->mark == _QS_IN_START, "Not in filter start()");
    struct QsStream *s = f->stream;
    DASSERT(s);
    ASSERT(s->flags & _QS_STREAM_START, "Stream is not starting");
    DASSERT(f->numInputs, "Filter \"%s\" has no inputs", f->name);
    DASSERT(!(s->flags & _QS_STREAM_STOP), "Stream is stopping");
    // This would be a user error.
    ASSERT(inPortNum >= f->numInputs);
    // This would be a user error.
    ASSERT(outPortNum >= f->numOutputs);

    DASSERT(f->readers);
    DASSERT(f->outputs);

    struct QsOutput *output = f->outputs + outPortNum;

    // This cannot be a pass-through output already.
    ASSERT(output->prev == 0,
            "filter \"%s\" output port %" PRIu32
            " is already a pass-through output",
            f->name, outPortNum);

    // We need to find the output that is feeding the reader at inPortNum.
    // At flow-time we don't need it, so we did not put it in the reader
    // (QsReader).
    //
    // We search the searched the whole stream to find the feeder filter's
    // output (QsOutput).
    //
    struct QsOutput *feedOutput = 0;
    struct QsFilter *feedFilter = 0;

    // Searching all filters in the stream
    for(uint32_t i=s->numSources-1; i!=-1; --i)
        if((feedOutput = FindFeedOutput(feedFilter = s->sources[i],
                        f, inPortNum)))
            break;

    if(feedOutput == 0)
        for(uint32_t i=s->numConnections-1; i!=-1; --i)
            if((feedOutput = FindFeedOutput(
                    feedFilter = s->connections[i].to,
                    f, inPortNum)))
                break;


    DASSERT(feedOutput);

    // If this feed output already has a "pass-through" buffer pointing to
    // it than some other filter already beat this filter, f, to making
    // this feed output a pass-through buffer.  Not getting the
    // pass-through setup may not be a fatal error, we leave it to the
    // filter module to decide.

    if(feedOutput->next) {
        ERROR("filter \"%s\" cannot make a pass-through buffer from "
                "input port%" PRIu32 " which is from filter \"%s\""
            " which is already a pass-through output",
            f->name, inPortNum, feedFilter->name);

        // This output already has a pass-through writer/reader thingy.
        return 1; // fail
    }


    // Now connect these to outputs in a doubly linked list with the feed
    // output being the prev of the filter, f, output.
    //
    output->prev = feedOutput;
    feedOutput->next = output;

    output->maxWrite = maxWriteLen;

    return 0; // success
}


#if 0
// TODO: Do we want this stupid function.
int
qsCreatePassThroughBufferDownstream(uint32_t outputPortNum,
        struct QsFilter *toFilter, uint32_t toInputPort) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    struct QsFilter *f = _qsCurrentFilter;
    DASSERT(f);
    DASSERT(f->numOutputs <= _QS_MAX_CHANNELS);
    DASSERT(toFilter->numInputs <= _QS_MAX_CHANNELS);
    DASSERT(f->numOutputs);
    DASSERT(!(f->stream->flags & _QS_STREAM_STOP), "We are stopping");
    // This function can be called in filter construct() or start().
    ASSERT(s->flags & _QS_STREAM_STOP || f-mark == _QS_IN_CONSTRUCT,
            "We are not in construct() or start()");


    ASSERT(0, "Write this function");

    return 0; // success
}
#endif


void qsRemoveDefaultFilterOptions(void) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    struct QsFilter *f = pthread_getspecific(_qsKey);
    DASSERT(f);
    ASSERT(f->mark == _QS_IN_CONSTRUCT, "Not in filter construct()");

    ASSERT(0, "Write this function");
}


const char* qsGetFilterName(void) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    struct QsFilter *f = pthread_getspecific(_qsKey);
    DASSERT(f);
    ASSERT(f->mark == _QS_IN_CONSTRUCT || f->mark == _QS_IN_DESTROY ||
            f->mark == _QS_IN_START || f->mark == _QS_IN_STOP);
    DASSERT(f->name);

    return f->name;
}
