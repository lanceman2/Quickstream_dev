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

#if 0
static inline void
AdvanceWritePtr(struct QsOutput *output, size_t len) {

    // We are in the correct thread to do this.
    struct QsWriter *writer = output->writer;
    struct QsBuffer *buffer = writer->buffer;

    DASSERT(len <= buffer->overhangLength,
            "len=%zu > buffer->overhangLength=%zu",
            len, buffer->overhangLength);

    if(writer->writePtr + len < buffer->mem + buffer->mapLength)
        writer->writePtr += len;
    else
        writer->writePtr += len - buffer->mapLength;
}


static inline void
AdvanceReadPtr(struct QsOutput *output, size_t len) {

    // We are in the correct thread to do this.
    struct QsBuffer *buffer = output->writer->buffer;

    DASSERT(len <= buffer->overhangLength,
            "len=%zu > buffer->overhangLength=%zu",
            len, buffer->overhangLength);

    if(output->readPtr + len < buffer->mem + buffer->mapLength)
        output->readPtr += len;
    else
        output->readPtr += len - buffer->mapLength;
}
#endif


// Allocate all buffers for all filter in the stream that have not been
// allocated already by the filter start() calling qsBufferCreate().  This
// sets up the default buffering arrangement.
//
void AllocateBuffer(struct QsFilter *f) {

    uint32_t i;
    for(i=0; i<f->numOutputs;++i)
        if(f->outputs[i].buffer == 0)
            break;

    if(i != f->numOutputs) {
        // We found some outputs without buffers.
        //
        struct QsBuffer *buffer = calloc(1, sizeof(*buffer));
        ASSERT(buffer, "calloc(1, %zu) failed", sizeof(*buffer));
        buffer->mapLength = QS_DEFAULT_MAXWRITELEN;
        uint32_t numPorts = 0;
        for(uint32_t i=0; i<f->numOutputs;++i)
            // Count the number of output ports that do not have a buffer.
            if(f->outputs[i].buffer == 0)
                ++numPorts;

        buffer->outputs = calloc(1, sizeof(*buffer->outputs)*(numPorts+1));
        ASSERT(buffer->outputs,
                "calloc(1, %zu) failed",
                sizeof(*buffer->outputs)*(numPorts+1));

        // 0 terminate buffer->outputs[] array
        buffer->outputs[numPorts] = 0;

        for(uint32_t i=f->numOutputs-1; i!=-1;--i)
            if(f->outputs[i].buffer == 0) {
                // buffer->outputs points back to outputs that use this
                // buffer.
                buffer->outputs[--numPorts] = &f->outputs[i];
                f->outputs[i].buffer = buffer;
            }
    }

    // Mark this filter as done allocating buffer.  mark should have been
    // set to false before all AllocateBuffer() calls.  This will stop
    // infinite recursion if there are loops in the stream.
    f->mark = true;

    // There may be filters down stream that do not have buffers yet, even
    // if this filter did have its' buffers.

    for(i=0; i<f->numOutputs;++i) {
        DASSERT(f->outputs[i].filter, "");

        if(f->outputs[i].filter->mark)
            // recurse
            AllocateBuffer(f->outputs[i].filter);
    }
}


void FreeBuffers(struct QsFilter *f) {

}


void MapRingBuffers(struct QsFilter *f) {

}


void UnmapRingBuffers(struct QsFilter *f) {

}




void qsOutputs(const size_t lens[]) {

}

void *qsGetBuffer(uint32_t outputPortNum, size_t maxLen) {

    return 0;
}

void qsAdvanceInputs(const size_t lens[]) {

}

void qsSetInputMax(const size_t lens[]) {

}

// Outputs can share the same buffer.  The list of output ports is
// in the QS_ARRAYTERM terminated array outputPortNums[].
//
// Here we just allocate the output buffer structure.  Later we will
// mmap() the ring buffers, after all the filter start()s are called.
//
void qsBufferCreate(size_t maxWriteLen,
        const uint32_t outputPortNums_in[]) {

    DASSERT(_qsCurrentFilter, "");
    ASSERT(_qsCurrentFilter->numOutputs <= QS_MAX_CHANNELS, "");
    uint32_t *outputPortNums = (uint32_t *) outputPortNums_in;
    uint32_t i;

    if(outputPortNums == QS_ALLPORTS) {
        outputPortNums = alloca(sizeof(uint32_t)*
                (_qsCurrentFilter->numOutputs+1));
        for(i=0;i<_qsCurrentFilter->numOutputs;++i)
            outputPortNums[i] = i;
        outputPortNums[i] = QS_ARRAYTERM;
        ASSERT(i <= QS_MAX_CHANNELS, "");
    }
    DASSERT(outputPortNums, "");
    DASSERT(outputPortNums[0] != QS_ARRAYTERM, "");

    struct QsBuffer *buffer = calloc(1, sizeof(*buffer));
    ASSERT(buffer, "calloc(1, %zu) failed", sizeof(*buffer));
    buffer->mapLength = maxWriteLen;

    for(i=0;outputPortNums[i] != QS_ARRAYTERM; ++i) {
        DASSERT(i<QS_MAX_CHANNELS, "");
        DASSERT(outputPortNums[i]<_qsCurrentFilter->numOutputs, "");
        DASSERT(_qsCurrentFilter->outputs[outputPortNums[i]].buffer == 0,
                "");
        _qsCurrentFilter->outputs[outputPortNums[i]].buffer = buffer;
    }
    buffer->outputs = calloc(1, sizeof(*buffer->outputs)*(i+1));
    ASSERT(buffer->outputs,
            "calloc(1, %zu) failed", sizeof(*buffer->outputs)*(i+1));
    buffer->outputs[i] = 0;
    while(--i != -1)
        buffer->outputs[i] = &(
                _qsCurrentFilter->outputs[outputPortNums[i]]);
}
