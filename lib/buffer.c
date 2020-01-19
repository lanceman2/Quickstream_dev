#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <alloca.h>
#include <pthread.h>

#include "./debug.h"
#include "./qs.h"
#include "../include/quickstream/filter.h"


// Allocate all buffer structures for all filters in the stream.
//
// This does not create the ring buffers.  That is done by mmap()ing
// memory later.  This allocates (struct QsBuffer) a buffer for top feed
// filters, and sets up pointers to them in "pass through" buffers in
// filters with "pass through" buffers.
//
// This function will recurse to filters that read output.  It is called
// with all source filters in the bottom of the function call stack.
//
void AllocateBuffer(struct QsFilter *f) {

    f->mark = false;

    DASSERT(f);
    DASSERT(f->numOutputs <= _QS_MAX_CHANNELS);
    DASSERT((f->outputs && f->numOutputs) ||
            (!f->outputs && !f->numOutputs));

    for(uint32_t i=0; i<f->numOutputs; ++i) {
        struct QsOutput *output = f->outputs + i;
        DASSERT(output->numReaders);
        DASSERT(output->readers);
        DASSERT(output->buffer == 0);

        if(output->prev) {
            // Set from a qsCreatePassThroughBuffer() call in the filter
            // (f) f->start() function call, just before this call.  This
            // is a "pass through" buffer so we will not be using buffer
            // pointers: output::buffer, and output::newBuffer.
            //
            DASSERT(output->buffer == 0);

            struct QsOutput *o = output;
            // Find the top feeding output.
            while(o->next) o = o->next;
            // now o is points to the owner or top feed output.
            // This "pass through" buffer will use the feed buffer.
            DASSERT(o->buffer);
            output->buffer = o->buffer;
            continue;
        } // else

        output->buffer = calloc(1,sizeof(*output->buffer));
        ASSERT(output->buffer, "calloc(1,%zu) failed",
                sizeof(*output->buffer));
    }

    // Now recurse down to a reader filter.
    for(uint32_t i=0; i<f->numOutputs; ++i) {
        struct QsOutput *output = f->outputs + i;
        for(uint32_t j=0; j<output->numReaders; ++j)
            if(output->readers[j].filter->mark)
                AllocateBuffer(output->readers[j].filter);
    }
}


// This is called when outputs exist.  This un-maps memory and frees the
// buffer structure.  The buffer and mappings do not necessarily exist
// when this is called.  This does not free the outputs or readers
// structs.
//
// Just this filters' buffers.  No recursion.
//
void FreeBuffers(struct QsFilter *f) {

    DASSERT(f);

    for(uint32_t i=0; i<f->numOutputs; ++i) {
        struct QsOutput *output = f->outputs + i;
        DASSERT(output->numReaders);
        DASSERT(output->readers);
        DASSERT(output->buffer);

        if(output->prev) {
            // This is a "pass through" buffer so there is nothing to
            // free.  This pointed to an up stream filter output buffer.
            output->buffer = 0;
            continue;
        }

        struct QsBuffer *b = output->buffer;
        // There is no failure mode (accept for ASSERT) between allocating
        // the buffer and mapping the memory, therefore:
        DASSERT(b->mem);
        DASSERT(b->mapLength);
        DASSERT(b->overhangLength);
#ifdef DEBUG
        // TODO: Is this really useful?
        memset(b->mem, 0, b->mapLength);
#endif
        // freeRingBuffer() calls munmap() ...
        freeRingBuffer(b->mem, b->mapLength, b->overhangLength);
#ifdef DEBUG
        memset(b, 0, sizeof(*b));
#endif
        free(b);
        output->buffer = 0;
    }
}


//TODO: rewrite this.
static inline
void CheckMakeRingBuffer(struct QsBuffer *b,
        size_t len, size_t overhang) {

    DASSERT(b);
    
    if(b->mem != 0) return;

    b->mapLength = len;
    b->overhangLength = overhang;
    // makeRingBuffer() will round up mapLength and
    // overhangLength to the nearest page.
    b->mem = makeRingBuffer(&b->mapLength, &b->overhangLength);
}


// This function will recurse.
//
void MapRingBuffers(struct QsFilter *f) {

    f->mark = false;

    for(uint32_t i=0; i<f->numOutputs; ++i) {
        struct QsOutput *output = f->outputs + i;

        if(output->prev == 0) {
            // This is NOT a pass through buffer. 
            DASSERT(output->buffer);
            CheckMakeRingBuffer(output->buffer, 1, 1);
            // Initialize the writer
            output->writePtr = output->buffer->mem;
        } else {
            // This is a pass through buffer.
            struct QsOutput *o = output->prev;
            // This is a "pass through" buffer so it does not have it's
            // own mapping.  We find the buffer that will map memory
            // looking up the "pass through" buffer list.
            while(o->prev) o = o->prev;
            // o is now the output with the buffer that owns the memory
            // for the buffer.  It's the original output that writes to
            // the buffer first in the series of pass through buffers that
            // share this buffers memory mapping.
            DASSERT(o->buffer);
            output->buffer = o->buffer;
            output->writePtr = output->buffer->mem;
        }

        DASSERT(output->numReaders);
        DASSERT(output->readers);

        for(uint32_t j=0; j<output->numReaders; ++j)
            // Initialize the readers
            output->readers[j].readPtr = output->buffer->mem;
    }


    for(uint32_t i=0; i<f->numOutputs; ++i) {
        struct QsOutput *output = f->outputs + i;
        DASSERT(output->readers);
        DASSERT(output->numReaders);
        for(uint32_t j=0; j<output->numReaders; ++j)
            if(output->readers[j].filter->mark)
                // Recurse
                MapRingBuffers(output->readers[j].filter);
    }
}


void qsOutput(uint32_t portNum, const size_t len) {



}


// TODO: rewrite (write) this.
void *qsGetOutputBuffer(uint32_t outputPortNum,
        size_t maxLen, size_t minLen) {

    // Get the filter.
    struct QsJob *job = pthread_getspecific(_qsKey);
    ASSERT(job, "thread_getspecific(_qsKey) failed");
    struct QsFilter *f = job->filter;
    DASSERT(f);
    DASSERT(maxLen);
    DASSERT(maxLen >= minLen);
    DASSERT(f->input);
    DASSERT(f->numOutputs);
    DASSERT(f->outputs);
    DASSERT(f->numOutputs >= outputPortNum);

    //pthread_cond_t *cond = f->cond;
    struct QsOutput *o = f->outputs + outputPortNum;
    DASSERT(o->readers);
    DASSERT(o->numReaders);
    pthread_mutex_t *mutex = f->mutex;

 
    if(mutex) {
        // There may be other threads accessing this output, o, structure
        // in this filter, f.
        CHECK(pthread_mutex_lock(mutex));
    }
    // Else: This is a lock-less ring buffer.  How nice.  There must not
    //       be more than one thread calling this input().


    uint8_t *ret = 0;

    // Check that the mmap()-ed memory is large enough.
    // Look in each 
    for(;o ;o=o->next){
        // Iterate through all readers of this output



        // Iterate through all readers of this output
        struct QsReader *rEnd = o->readers + o->numReaders;
        for(struct QsReader *r=o->readers; r<rEnd; ++r) {

        }
    }


    if(mutex) {
        // There may be other threads accessing the buffer.
        if(maxLen == minLen)
            // Since the memory pointer positions are known we can
            // let other threads get new write pointer positions
            // by releasing this mutex lock and allowing parallel
            // processing in this filter.
            CHECK(pthread_mutex_unlock(mutex));
    }


    return ret;
}


void qsAdvanceInput(uint32_t inputPortNum, size_t len) {

}


void qsSetInputThreshold(uint32_t inputPortNum, size_t len) {

}



void qsSetInputReadPromise(uint32_t inputPortNum, size_t len) {

}


// Outputs can share the same buffer.  The list of output ports is
// in the QS_ARRAYTERM terminated array outputPortNums[].
//
// Here we just allocate the output buffer structure.  Later we will
// mmap() the ring buffers, after all the filter start()s are called.
//
void qsCreateOutputBuffer(uint32_t outputPortNum, size_t maxWriteLen) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    DASSERT(_qsCurrentFilter);
    ASSERT(_qsCurrentFilter->numOutputs <= _QS_MAX_CHANNELS);
    ASSERT(_qsCurrentFilter->stream->flags & _QS_STREAM_START,
            "We are not starting");
    DASSERT(_qsCurrentFilter->numOutputs);
    DASSERT(!(_qsCurrentFilter->stream->flags & _QS_STREAM_STOP),
            "We are stopping");


    ASSERT(0, "qsCreateOutputBuffer() is not written yet");
}


int
qsCreatePassThroughBuffer(uint32_t inPortNum, uint32_t outputPortNum,
        size_t maxWriteLen) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    DASSERT(_qsCurrentFilter);
    DASSERT(_qsCurrentFilter->mark == 0);
    ASSERT(_qsCurrentFilter->numOutputs <= _QS_MAX_CHANNELS);
    ASSERT(_qsCurrentFilter->stream->flags & _QS_STREAM_START,
            "We are not starting");
    DASSERT(_qsCurrentFilter->numOutputs);
    DASSERT(!(_qsCurrentFilter->stream->flags & _QS_STREAM_STOP),
            "We are stopping");


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
