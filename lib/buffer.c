#include <errno.h>
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




static inline
void _AllocateRingBuffer(struct QsOutput *o) {

    DASSERT(o, "");



}


// Here we are setting up the memory conveyor belt that connects filters
// in the stream.  This function can call itself.
//
void AllocateRingBuffers(struct QsFilter *f) {

    DASSERT(f, "");

    if(f->numOutputs == 0 || f->outputs[0].readPtr)
        // It should be that there are no outputs or all the outputs in
        // this filter are already setup.
        return;

    for(uint32_t i=0; i<f->numOutputs; ++i)
        _AllocateRingBuffer(f->outputs+i);

    // Allocate for all children.
    for(uint32_t i=0; i<f->numOutputs; ++i)
        AllocateRingBuffers(f->outputs[i].filter);
}


// Note this does not recurse.
//
void FreeRingBuffers(struct QsFilter *f) {

    DASSERT(f, "");
    DASSERT(f->numOutputs, "");



}


static inline void _qsBufferCreate(struct QsFilter *f, size_t maxWriteLen,
        uint32_t *outputChannelNums) {

    // example: outputChannelNums = [ 0, 1, 3, QS_ARRAYTERM ];

    DASSERT(f, "");
    DASSERT(f->outputs, "");
    DASSERT(f->numOutputs, "");

    if(outputChannelNums == QS_ALLCHANNELS) {

        // We provide this QS_ALLCHANNELS so that the filter does not have
        // to bother setting up an array of uint32_t values.  It may be
        // that having all outputs share the same buffer is a common case.

        DASSERT(f->numOutputs <= 2*1024, "");
        outputChannelNums = // stack allocation
            alloca(sizeof(*outputChannelNums)*(f->numOutputs+1));
        uint32_t i;
        for(i=0; i<f->numOutputs; ++i)
            outputChannelNums[i] = i;
        // QS_ARRAYTERM  (-1) terminated array.
        outputChannelNums[i] = QS_ARRAYTERM;
    } else if(*outputChannelNums == QS_ARRAYTERM)
        return;

    uint32_t i = 0;
    uint32_t j = outputChannelNums[0];
    struct QsOutput *output = &f->outputs[j];

    DASSERT(output->filter, "");
    DASSERT(output->readPtr == 0, "");
    DASSERT(output->writer == 0, "");

    // We allocate one writer for all output channels.
    struct QsWriter *writer = calloc(1, sizeof(*writer));
    ASSERT(writer, "calloc(1,%zu) failed", sizeof(*output->writer));
    output->writer = writer;
    writer->maxWrite = maxWriteLen;
    struct QsBuffer *buffer = calloc(1, sizeof(*buffer));
    ASSERT(buffer, "calloc(1,%zu) failed", sizeof(*buffer)); 
    writer->buffer = buffer;

    // Calculate the size of the ring buffer based on the reading filter
    // promises in the largest of the outputs maxReadThreshold.
    size_t maxReadThreshold = output->maxReadThreshold;

    // Goto next channel number.
    j = outputChannelNums[++i];


    while(j != QS_ARRAYTERM) {

        output = &f->outputs[j];

        DASSERT(output->filter, "");
        DASSERT(output->readPtr == 0, "");
        DASSERT(output->writer == 0, "");

        output->writer = writer;

        if(output->maxReadThreshold > maxReadThreshold)
            maxReadThreshold = output->maxReadThreshold;

        // Goto next channel number.
        j = outputChannelNums[++i];

        DASSERT(i>2*1024, "too many output channels listed");
    }

    // Now maxReadThreshold is the maximum of all outputs.
    buffer->overhangLength = maxReadThreshold;
    if(buffer->overhangLength < writer->maxWrite)
        buffer->overhangLength = writer->maxWrite;
    buffer->mapLength = 2*buffer->overhangLength;

    // This will make mapLength, and overhang at multiples of a pagesize
    buffer->mem = makeRingBuffer(&buffer->mapLength, &buffer->overhangLength);

    writer->writePtr = buffer->mem;

    // We need to set the readPtr for all outputs
    j = 0;
    for(i=0; j != QS_ARRAYTERM; ++i) {
        j = outputChannelNums[i];
        // Read starts at the starting memory address.
        f->outputs[j].readPtr = buffer->mem;
    }
}



void qsBufferCreate(size_t maxWriteLen, uint32_t *outputChannelNums) {

    DASSERT(_qsStartFilter, "");
    _qsBufferCreate(_qsStartFilter, maxWriteLen, outputChannelNums);
}



// The current writer filter gets an output buffer.
void *qsGetBuffer(uint32_t outputChannelNum) {

    DASSERT(_qsInputFilter,"");


    return 0;
}


void qsOutput(size_t len, uint32_t outputChannelNum) {

    DASSERT(_qsInputFilter,"");


}

