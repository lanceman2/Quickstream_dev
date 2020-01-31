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
/////////////////////////////////////////////////////////////////////////
// In this file is the implementation of most of C functions in the
// quickstream filter module writers API (application programming
// interface).
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


// This is boiler plate that is at the top of some functions in this file.
static inline
struct QsJob *GetJob(void) {

    DASSERT(_qsMainThread != pthread_self(), "Is main thread");
    struct QsJob *j = pthread_getspecific(_qsKey);
    // If job, j, was not in thread specific data than this could
    // be due to a user calling this function while not in a filter module
    // input() call.
    ASSERT(j, "Not from code in a filter module input() function");
    // Double check if DEBUG
    DASSERT(j->magic == _QS_IS_JOB);

#ifdef DEBUG
    struct QsFilter *f = j->filter;
    DASSERT(f);
    struct QsStream *s = f->stream;
    DASSERT(s);
    DASSERT(!(s->flags & _QS_STREAM_START), "Stream is starting");
    DASSERT(!(s->flags & _QS_STREAM_STOP), "Stream is stopping");
    DASSERT(f->mark, "This finished filter \"%s\" "
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

    DASSERT(f->numOutputs, "Filter \"%s\" has no outputs",
            f->name);
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

    // This is all this function needed to do.
    j->outputLens[outputPortNum] += len;

    // Another user error:
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

    // TODO:
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

    ASSERT(0, "Write this code");
}


void qsSetInputThreshold(uint32_t inputPortNum, size_t len) {

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

    f->readers[inputPortNum]->maxRead = len;
}


// Outputs can share the same buffer.  The list of output ports is
// in the QS_ARRAYTERM terminated array outputPortNums[].
//
// Here we just allocate the output buffer structure.  Later we will
// mmap() the ring buffers, after all the filter start()s are called.
//
void qsCreateOutputBuffer(uint32_t outputPortNum, size_t maxWriteLen) {

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
    ASSERT(outputPortNum < f->numOutputs);
    DASSERT(f->outputs);

    ASSERT(0, "qsCreateOutputBuffer() is not written yet");
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

    ASSERT(0, "Write this function");
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
