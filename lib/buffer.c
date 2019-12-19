#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <alloca.h>

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

    DASSERT(f, "");
    DASSERT(f->numOutputs <= _QS_MAX_CHANNELS, "");
    DASSERT((f->outputs && f->numOutputs) ||
            (!f->outputs && !f->numOutputs), "");

    for(uint32_t i=0; i<f->numOutputs; ++i) {
        struct QsOutput *output = f->outputs + i;
        DASSERT(output->numReaders, "");
        DASSERT(output->readers, "");
        DASSERT(output->newBuffer == 0, "");
        DASSERT(output->buffer == 0, "");

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


// This is called when outputs exist, and after un-mapping memory.
//
// Just this filters' buffers.  No recursion.
//
void FreeBuffers(struct QsFilter *f) {
    
    DASSERT(f, "");

    for(uint32_t i=0; i<f->numOutputs; ++i) {
        struct QsOutput *output = f->outputs + i;
        DASSERT(output->numReaders, "");
        DASSERT(output->readers, "");
        DASSERT(output->newBuffer == 0, "");
        DASSERT(output->buffer, "");

        if(output->prev) {
            // This is a "pass through" buffer so we do not free it yet.
            // This is freed by the output that owns this buffer.
            output->buffer = 0;
            continue;
        }
#ifdef DEBUG
        memset(output->buffer, 0,  sizeof(*output->buffer));
#endif
        free(output->buffer);
        output->buffer = 0;
    }
}


// This function will recurse.
//
void MapRingBuffers(struct QsFilter *f) {

    for(uint32_t i=0; i<f->numOutputs; ++i) {
        struct QsOutput *output = f->outputs + i;

        if(output->prev) {

            // This is a pass through buffer and it will not be set yet.
            DASSERT(output->buffer == 0, "");

            struct QsOutput *o = output->prev;
            // This is a "pass through" buffer so it does not have it's
            // own mapping.  We find the buffer that will map memory
            // looking up the "pass through" buffer list.
            while(o->prev) o = o->prev;
            // o is now the output with the buffer that owns the memory
            // for the buffer.  It's the original output that writes to
            // the buffer first in the series of pass through buffers that
            // share this buffer.
            DASSERT(o->buffer, "");
            if(o->buffer->mem == 0) {
                o->buffer->mapLength = 1;
                o->buffer->overhangLength = 1;
                // makeRingBuffer() will round up mapLength and
                // overhangLength to the nearest page.
                o->buffer->mem = makeRingBuffer(
                        &o->buffer->mapLength, &o->buffer->overhangLength);
            }
            output->buffer = o->buffer;
        }

        DASSERT(output->buffer, "");
        
        if(output->buffer->mem == 0) {
            output->buffer->mapLength = 1;
            output->buffer->overhangLength = 1;
            // makeRingBuffer() will round up mapLength and
            // overhangLength to the nearest page.
            output->buffer->mem = makeRingBuffer(
                    &output->buffer->mapLength,
                    &output->buffer->overhangLength);
        }

        // Initialize the writer
        output->writePtr = output->buffer->mem;

        for(uint32_t j=0; j<output->numReaders; ++j)
            output->readers[j].readPtr = output->buffer->mem;
    }
}


// Just this filters' buffers' mappings.
//
void UnmapRingBuffers(struct QsFilter *f) {

    DASSERT(f->outputs, "");

}


void qsOutput(uint32_t portNum, const size_t len) {



}


void *qsGetOutputBuffer(uint32_t outputPortNum,
        size_t maxLen, size_t minLen) {

    return 0;
}


void qsAdvanceInput(uint32_t inputPortNum, size_t len) {

}


// Outputs can share the same buffer.  The list of output ports is
// in the QS_ARRAYTERM terminated array outputPortNums[].
//
// Here we just allocate the output buffer structure.  Later we will
// mmap() the ring buffers, after all the filter start()s are called.
//
void qsCreateOutputBuffer(uint32_t outputPortNum, size_t maxWriteLen) {

    DASSERT(_qsCurrentFilter, "");
    ASSERT(_qsCurrentFilter->numOutputs <= _QS_MAX_CHANNELS, "");
    DASSERT(_qsCurrentFilter->numOutputs, "");

    ASSERT(0, "qsCreateOutputBuffer() is not written yet");
}


void
qsCreatePassThroughBuffer(uint32_t inPortNum, uint32_t outputPortNum,
        size_t maxWriteLen) {

}
