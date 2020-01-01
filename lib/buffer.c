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

#include "./qs.h"
#include "../include/qsfilter.h"
#include "./debug.h"


// Allocate all buffers for all filters in the stream that have not been
// allocated already by the filter start() calling qsBufferCreate().  This
// sets up the default buffering arrangement.
//
// This function will recurse to filters that read output.  It is called
// with all source filters.
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
        DASSERT(output->newBuffer == 0);
        DASSERT(output->buffer == 0);

        if(output->prev)
            // Set from a qsCreatePassThroughBuffer() call in the filter
            // (f) f->start() function call, just before this call.  This
            // is a "pass through" buffer so we do not allocate it yet.
            continue;

        output->buffer = calloc(1,sizeof(*output->buffer));
        ASSERT(output->buffer, "calloc(1,%zu) failed",
                sizeof(*output->buffer));
    }

    // Now recurse.
    for(uint32_t i=0; i<f->numOutputs; ++i) {
        struct QsOutput *output = f->outputs + i;
        for(uint32_t j=0; j<output->numReaders; ++j)
            if(output->readers[j].filter->mark)
                AllocateBuffer(output->readers[j].filter);
    }
}


// This is called when outputs exist.  This un-maps memory and frees the
// buffer structure.  The buffer and mappings do not necessarily exist
// when this is called.
//
// Just this filters' buffers.  No recursion.
//
void FreeBuffers(struct QsFilter *f) {

    DASSERT(f);

    for(uint32_t i=0; i<f->numOutputs; ++i) {
        struct QsOutput *output = f->outputs + i;
        DASSERT(output->numReaders);
        DASSERT(output->readers);
        DASSERT(output->newBuffer == 0);
        DASSERT(output->buffer);

        if(output->prev) {
            // This is a "pass through" buffer so we do not free it yet.
            // This is freed by the output that owns this buffer.
            output->buffer = 0;
            continue;
        }

        struct QsBuffer *b = output->buffer;
        if(b) {
            // There is no failure mode between allocating the buffer and
            // mapping the memory, therefore:
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
}


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

        if(output->prev) {

            // This is a pass through buffer and it will not be set yet.
            DASSERT(output->buffer == 0);

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
            CheckMakeRingBuffer(o->buffer, 1, 1);
            output->buffer = o->buffer;
        }

        DASSERT(output->buffer);
        CheckMakeRingBuffer(output->buffer, 1, 1);

        // Initialize the writer
        output->oldWritePtr = output->writePtr = output->buffer->mem;

        DASSERT(output->numReaders && output->readers);
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
    pthread_mutex_t *mutex = o->mutex;

    // First question: can there be more than one thread calling this
    // function for this filter?
    //
    //   Yes, mutex is non-zero.
    //
    //   No, mutex is zero.
    //

    if(mutex) {
        // There may be other threads accessing this output (o) structure.
        CHECK(pthread_mutex_lock(mutex));
    }
    // Else: This is a lock-less ring buffer.  How nice.  There must not
    //       be more than one thread calling this input().



    // Second question: What kind of buffer is this?
    //
    //   1. "Head buffer" = not a pass through
    //
    //   2. "Pass-through buffer"
    //



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


#if 0
// Outputs can share the same buffer.  The list of output ports is
// in the QS_ARRAYTERM terminated array outputPortNums[].
//
// Here we just allocate the output buffer structure.  Later we will
// mmap() the ring buffers, after all the filter start()s are called.
//
void qsCreateOutputBuffer(uint32_t outputPortNum, size_t maxWriteLen) {

    DASSERT(_qsCurrentFilter);
    ASSERT(_qsCurrentFilter->numOutputs <= _QS_MAX_CHANNELS);
    DASSERT(_qsCurrentFilter->numOutputs);

    ASSERT(0, "qsCreateOutputBuffer() is not written yet");
}
#endif


int
qsCreatePassThroughBuffer(uint32_t inPortNum, uint32_t outputPortNum,
        size_t maxWriteLen) {

    ASSERT(0, "Write this function");
    return 0; // success
}


int
qsCreatePassThroughBufferDownstream(uint32_t outputPortNum,
        struct QsFilter *toFilter, uint32_t toInputPort) {

    ASSERT(0, "Write this function");
    return 0; // success
}
