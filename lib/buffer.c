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
}


// This is called when outputs exist, and after un-mapping memory.
//
// Just this filters' buffers.
//
void FreeBuffers(struct QsFilter *f) {
    
    DASSERT(f->outputs, "");

}

// This function will recurse.
//
void MapRingBuffers(struct QsFilter *f) {

    DASSERT(f->outputs || f->numOutputs == 0, "");
    DASSERT(f->numOutputs <= _QS_MAX_CHANNELS, "");

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
