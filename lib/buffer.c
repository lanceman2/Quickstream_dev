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




// TODO: This function may have to be broken into 2 stages of for-loops,
// so that we may add pass-through buffers.  One loop-stage will get the
// buffer parameters, and the next loop-stage will do the allocation.
//
// This function is the part of AllocateRingBuffers() that does not call
// itself.
//
static inline
void _AllocateRingBuffers(struct QsFilter *f) {

    DASSERT(f, "");
    DASSERT(f->numOutputs, "");

    struct QsOutput *outputs = f->outputs;
    DASSERT(outputs, "");

    // At this point all the filter start() functions have been called.
    //
    // So they made their buffer configuration requests, setting stuff
    // like (writer) maxWrite and (reader) maxReadThreshold, or they are
    // the default values for these buffer parameters.

    // At this point the filter had there chance to configure ring buffer
    // connectivity.  Any filter->outputs[] struct without a writer will
    // will use a shared writer that will be created here.

    // defaultWriter will be the allocated writer if we find any outputs
    // that do not have buffers and writers.
    //
    struct QsWriter *defaultWriter = 0;

    for(uint32_t i=0; i<f->numOutputs; ++i) {
        struct QsOutput *output = &f->outputs[i];
        if(output->writer == 0) {
            if(!defaultWriter) {
                defaultWriter = calloc(1, sizeof(*defaultWriter));
                ASSERT(defaultWriter, "calloc(1,%zu) failed",
                        sizeof(*defaultWriter));
                defaultWriter->maxWrite = QS_DEFAULTWRITELENGTH;
            }
            output->writer = defaultWriter;
        }
    }

    // Now all ouputs have writers.

    // We need to find outputs grouped by sharing the same writer,
    // including the one we just created above.  For each writer we
    // allocate what is needed to complete the struct qsOutput data
    // structure and it's pointers to other data like QSWriter and
    // QsBuffer.
    //
    for(uint32_t i=0; i<f->numOutputs; ++i) {
        
        struct QsWriter *writer = outputs[i].writer;

        if(writer->buffer)
            // This writer already has a buffer and therefore should be
            // all set to go.
            continue;

        struct QsBuffer *buffer = calloc(1, sizeof(*buffer));
        ASSERT(buffer, "calloc(1,%zu) failed", sizeof(*buffer));

        writer->buffer = buffer;
        DASSERT(writer->maxWrite, "maxWrite cannot be 0");
        // The overhang length will be the largest of any single write or
        // read operation.
        buffer->overhangLength = writer->maxWrite;
        buffer->mapLength = writer->maxWrite;

        size_t maxReadLength = 0;
 
        // Get the buffer size parameters.
        //

        for(uint32_t j=0; j<f->numOutputs; ++j) {

            if(outputs[j].writer == writer) {

                DASSERT(outputs[j].maxReadThreshold, "");

                if(maxReadLength < outputs[j].maxReadThreshold)
                    maxReadLength = outputs[j].maxReadThreshold;
            }
        }
    }
}


// Here we are setting up the memory conveyor belt that connects filters
// in the stream.  This function calls itself.
//
void AllocateRingBuffers(struct QsFilter *f) {

    DASSERT(f, "");

    if(f->numOutputs == 0 || f->outputs[0].readPtr)
        // It should be that there are no outputs or all the outputs in
        // this filter are already setup.
        return;

    _AllocateRingBuffers(f);

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


static inline void BufferWriterCreate(struct QsFilter *f, size_t maxWriteLen,
        uint32_t *outputChannelNums) {

    // example: outputChannelNums = [ 0, 1, 3, QS_ARRAYTERM ];
    //
    // At this time we cannot allocate the buffers yet because the filters
    // that we are outputting to may not have had a chance to configure
    // their inputs yet by having their start() called.  So for example
    // parameters like maxReadThreshold may not have been set by the
    // reading filter yet.
    //
    // So we just allocate the QsWrite to mark it as part of this shared
    // buffer group without allocating the ring buffer memory yet, since
    // we cannot be sure of its size yet.

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
        return; // This should not happen, but it's not bad because we are
                // handling it now by returning.  Pointless call with no
                // output channels.

    uint32_t i = 0;
    uint32_t j = outputChannelNums[0];
    struct QsOutput *output = &f->outputs[j];

    DASSERT(output->filter, "");
    DASSERT(output->readPtr == 0, "");
    DASSERT(output->writer == 0, "");

    // We allocate one writer for all output channels that where passed
    // in.
    struct QsWriter *writer = calloc(1, sizeof(*writer));
    ASSERT(writer, "calloc(1,%zu) failed", sizeof(*output->writer));
    output->writer = writer;
    writer->maxWrite = maxWriteLen;
    // TODO: At this point the buffer could have been part of the writer
    // structure, but we do want to add the sharing of buffers between
    // writers in adjacent filters.  We call it pass through buffer sharing
    // between filters.

    // Goto next channel number.
    j = outputChannelNums[++i];

    while(j != QS_ARRAYTERM) {

        output = &f->outputs[j];

        DASSERT(output->filter, "");
        DASSERT(output->readPtr == 0, "");
        DASSERT(output->writer == 0, "");

        // Mark this output as part of this writer group.
        output->writer = writer;

        // Goto next channel number.
        j = outputChannelNums[++i];

        DASSERT(i>2*1024, "too many output channels listed");
    }
}



void qsBufferCreate(size_t maxWriteLen, uint32_t *outputChannelNums) {

    DASSERT(_qsStartFilter, "");
    BufferWriterCreate(_qsStartFilter, maxWriteLen, outputChannelNums);
}



// The current writer filter gets an output buffer so it may write to the
// buffer to an output.
void *qsGetBuffer(uint32_t outputChannelNum) {

    DASSERT(_qsInputFilter,"");


    return 0;
}


void qsOutput(size_t len, uint32_t outputChannelNum) {

    DASSERT(_qsInputFilter,"");


}

