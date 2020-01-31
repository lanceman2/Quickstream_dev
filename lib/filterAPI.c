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
// In this file is the implementation of most of C functions in the
// quickstream filter module writers API (application programming
// interface).
/////////////////////////////////////////////////////////////////////////


void qsOutput(uint32_t portNum, const size_t len) {



}


void qsAdvanceInput(uint32_t inputPortNum, size_t len) {

}


void qsSetInputThreshold(uint32_t inputPortNum, size_t len) {

}



void qsSetInputReadPromise(uint32_t inputPortNum, size_t len) {

    // We only call this in the main thread in start().
    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    struct QsFilter *f = pthread_getspecific(_qsKey);
    DASSERT(f);
    ASSERT(f->mark == _QS_IN_START, "Not in filter start()");
    struct QsStream *s = f->stream;
    DASSERT(s);
    ASSERT(f->stream->flags & _QS_STREAM_START, "Stream is not starting");
    DASSERT(f->numInputs, "Filter \"%s\" has no inputs", f->name);
    DASSERT(!(s->flags & _QS_STREAM_STOP), "Stream is stopping");
    // This would be a user error.
    ASSERT(inputPortNum >= f->numInputs);
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
    ASSERT(f->stream->flags & _QS_STREAM_START, "Stream is not starting");
    DASSERT(f->numInputs, "Filter \"%s\" has no inputs", f->name);
    DASSERT(!(s->flags & _QS_STREAM_STOP), "Stream is stopping");
    // This would be a user error.
    ASSERT(outputPortNum >= f->numOutputs);
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
    ASSERT(f->stream->flags & _QS_STREAM_START, "Stream is not starting");
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
