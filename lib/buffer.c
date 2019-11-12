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
    // So they made their buffer configuration requests, stuff like
    // (writer) maxWrite and (reader) maxReadThreshold has been set, or
    // they are the default values for these buffer parameters.

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
                defaultWriter->maxWrite = QS_DEFAULT_MAXWRITE;
            }
            output->writer = defaultWriter;
            ++output->writer->refCount;
        }
    }

    // Now all outputs have writers.
    //
    // Now we have no buffers.

    // TODO: pass-through buffers...

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
            // all set to go.  TODO: pass-through buffers...
            continue;

        struct QsBuffer *buffer = calloc(1, sizeof(*buffer));
        ASSERT(buffer, "calloc(1,%zu) failed", sizeof(*buffer));

        writer->buffer = buffer;
        //TODO: pass-through buffers with need this refCount
        // For now there is one writer per buffer.
        // THIS CODE DOES NOT SUPPORT pass-through buffers yet.
        //
        ++buffer->refCount;
        DASSERT(writer->maxWrite, "maxWrite cannot be 0");
        // The overhang length will be the largest of any single write or
        // read operation.
        buffer->overhangLength = writer->maxWrite;
        buffer->mapLength = writer->maxWrite;

        size_t maxInput = 0;
 
        // Get the buffer size parameters.
        //
        for(uint32_t j=0; j<f->numOutputs; ++j) {

            // NO GOOD FOR pass-through buffers yet.
            if(outputs[j].writer == writer) {

                DASSERT(outputs[j].inputThreshold, "");

                if(maxInput < outputs[j].maxInput)
                    maxInput = outputs[j].maxInput;
            }
        }
        // Now we have the max Read Length
        //
        // The size of the overhang is the largest single read or write
        // operation.  TODO: not so for pass-through
        //
        if(maxInput > writer->maxWrite)
            buffer->overhangLength = maxInput;
        else
            buffer->overhangLength = writer->maxWrite;

        buffer->mapLength = writer->maxWrite + maxInput;

        // makeRingBuffer() will make mapLength and overhangLength be the
        // next multiple of the system pagesize (currently 4*1024) in
        // bytes.
        buffer->mem = makeRingBuffer(&buffer->mapLength,
                &buffer->overhangLength);

        // Set all the output read and write buffer mem pointers
        //
        for(uint32_t j=0; j<f->numOutputs; ++j) {

            // All outputs that use this ring buffer:
            if(outputs[j].writer->buffer == buffer) {
                // Set the read pointer to the start of ring buffer memory
                outputs[j].readPtr = buffer->mem;
                if(outputs[j].writer->writePtr == 0)
                    // Set the write pointer to the start of ring buffer memory
                    outputs[j].writer->writePtr = buffer->mem;
#ifdef DEBUG
                else
                    DASSERT(outputs[j].writer->writePtr == buffer->mem, "");
#endif
            }
        }
    }
}


// Here we are setting up the memory conveyor belt that connects filters
// in the stream.  This function calls itself.
//
void AllocateRingBuffers(struct QsFilter *f) {

    DASSERT(f, "");
    DASSERT(f->stream, "");

    if(f->numOutputs == 0 || f->outputs[0].readPtr) {
        // It should be that there are no outputs or all the outputs in
        // this filter are already setup because there are loops in the
        // stream flow.
        DASSERT(f->numOutputs == 0 || f->stream->flags & _QS_STREAM_ALLOWLOOPS,
                "Found loops in the flow when there should be none.");
        return;
    }


    _AllocateRingBuffers(f);

    // Allocate for all children.
    for(uint32_t i=0; i<f->numOutputs; ++i)
        AllocateRingBuffers(f->outputs[i].filter);
}


static inline void FreeBuffer(struct QsBuffer *buffer) {

    DASSERT(buffer->mem, "");
    freeRingBuffer(buffer->mem, buffer->mapLength, buffer->overhangLength);
#ifdef DEBUG
    memset(buffer, 0, sizeof(*buffer));
#endif
    free(buffer);
    DSPEW("Freed buffer");
}


static inline void FreeWriter(struct QsWriter *writer) {

    DASSERT(writer, "");
    DASSERT(writer->refCount == 0, "");
    DASSERT(writer->buffer, "");
    DASSERT(writer->buffer->refCount, "");

    --writer->buffer->refCount;

    if(writer->buffer->refCount == 0)
        // No more writers are using this buffer.
        FreeBuffer(writer->buffer);

#ifdef DEBUG
    memset(writer, 0, sizeof(*writer));
#endif
    free(writer);
}


// Note this does not recurse.
//
// This gets called by every filter in the stream.
//
// This frees the writer and buffer structs, and ringBuffer memory
// mappings, via static functions above.
//
// The QsOutput structs are freed just after this call.
//
// We must free in this order (which is the reverse of allocation order):
//
//    1. struct QsBuffer with its' mapping
//    2. struct QsWriter
//    3. struct QsOutput (after this call)
//
// because they are pointing to each other in reverse of that order and
// they all can share children between siblings.  Sharing children: that
// is a QsOutput can point to the same QsWriter as another QsOutput
// (intra-filter shared buffer output), in the same as a QsWriter can
// point to the same QsBuffer (for inter-filter pass-through buffer).
//
// We did not bother with pointers that point the other direction because
// they are not needed at run-time.  This causes extra looping code, but
// not at the steady flow state run-time.
//
void FreeRingBuffersAndWriters(struct QsFilter *f) {

    DASSERT(f, "");
    DASSERT(f->stream, "");
    struct QsOutput *outputs = f->outputs;
    uint32_t numOutputs = f->numOutputs;
    DASSERT(numOutputs, "");
    DASSERT(outputs, "");

    // Loop over all outputs in this filter.
    for(uint32_t j=0; j<f->numOutputs; ++j) {
        struct QsOutput *output = &f->outputs[j];
        DASSERT(output, "");
        struct QsWriter *writer = output->writer;
        DASSERT(writer, "");
        struct QsBuffer *buffer = writer->buffer;
        DASSERT(buffer, "");
        --writer->refCount;
        if(writer->refCount == 0)
            FreeWriter(writer);
    }
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
    // We allocate the QsWrite to mark it as part of this shared buffer
    // group without allocating the ring buffer memory yet, since we do
    // not know the ring buffer size yet.

    DASSERT(f, "");
    DASSERT(f->outputs, "");
    DASSERT(f->numOutputs, "");

    if(outputChannelNums == QS_ALLCHANNELS) {

        // We provide this QS_ALLCHANNELS so that the filter does not have
        // to bother setting up an array of uint32_t values.  It may be
        // that having all outputs share the same buffer is a common case.

        // This should have been checked in stream.c so we just check on
        // DEBUG now.
        DASSERT(f->numOutputs <= QS_MAX_CHANNELS, "");

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
    ++writer->refCount;
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
        ++writer->refCount;

        // Goto next channel number.
        j = outputChannelNums[++i];

        DASSERT(i>QS_MAX_CHANNELS, "too many output channels listed");
    }
}


void qsBufferCreate(size_t maxWriteLen, uint32_t *outputChannelNums) {

    // _qsCurrentFilter is the filter that is having start() called. 
    DASSERT(_qsCurrentFilter, "");
    BufferWriterCreate(_qsCurrentFilter, maxWriteLen, outputChannelNums);
}


static inline
void CheckWriterBufferLength(struct QsWriter *w, size_t maxLen) {
    
    // All lengths in bytes.
    //
    // 1. maxLen must not exceed the ring buffer overhangLength, and
    //
    // 2. all current unread lengths plus maxLen must not be greater
    //    than mapLength.
    //
    struct QsBuffer *b = w->buffer;

    // Lengths if needed.
    size_t overhangLength = 0, mapLength = 0;

    if(maxLen > b->overhangLength)
        overhangLength = maxLen;

    
}



// The current writer filter gets an output buffer so it may write to the
// buffer to an output.  This write pointer is only accessed by this
// filters thread.
void *qsGetBuffer(uint32_t outputChannelNum, size_t maxLen) {

    DASSERT(_input.filter,"");
    DASSERT(outputChannelNum < _input.filter->numOutputs, "");

#ifdef DEBUG
    struct QsWriter *writer = 
        _input.filter->outputs[outputChannelNum].writer;
    DASSERT(writer->writePtr < writer->buffer->mem +
            writer->buffer->mapLength, "");
    DASSERT(writer->writePtr >= writer->buffer->mem, "");
    return writer->writePtr;
#else
    return _input.filter->outputs[outputChannelNum].writer->writePtr;
#endif
}


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


extern
size_t GetReadLength(struct QsOutput *output) {

    // We are in the correct thread to do this.
    struct QsWriter *writer = output->writer;

    if(writer->writePtr >= output->readPtr)
        return writer->writePtr - output->readPtr;
    else {
        DASSERT(writer->buffer->mapLength > 
                output->readPtr - writer->writePtr, "");
        // output->readPtr > writer->writePtr
        return writer->buffer->mapLength +
            writer->writePtr - output->readPtr;
    }
}



// Here we just advance the write pointer.  This write pointer is only
// accessed by this filters thread. 
//
void _qsOutput(size_t len, uint32_t outputChannelNum) {

    DASSERT(_input.filter,"");
    DASSERT(_input.filter->numOutputs > outputChannelNum, "");
    struct QsWriter *writer =
        _input.filter->outputs[outputChannelNum].writer;

    DASSERT(writer->writePtr < writer->buffer->mem +
            writer->buffer->mapLength, "");
    DASSERT(writer->writePtr >= writer->buffer->mem, "");

    ASSERT(len <= writer->maxWrite, "");

    if(len)
        AdvanceWritePtr(&_input.filter->outputs[outputChannelNum], len);

    // Check all the outputs with this writer.
    // TODO: Make this not check all the outputs.
    for(uint32_t i=0; i<_input.filter->numOutputs; ++i) {

        if(_input.filter->outputs[i].writer == writer) {

            struct QsOutput *output = &_input.filter->outputs[i];

            // This is the only thread that will access readPtr(s) and
            // writePtr.  We pass the pointer to ring buffer memory on the
            // stack in this sendOutput() function call.

            // We computer the length that we have available to read.
            // There can be accumulated data in the buffer from previous
            // qsOutput() calls.
            size_t accLen = GetReadLength(output);
            // Check for buffer overrun.
            ASSERT(accLen <= output->writer->buffer->mapLength,
                    "buffer overrun");

            if(accLen >= output->minReadThreshold) {
                // Stack memory to keep state.
                uint32_t returnFlowState = _input.flowState;
                size_t lenConsumed =
                    _input.filter->outputs[i].filter->sendOutput(
                            _input.filter->outputs[i].filter,
                            &_input.filter->outputs[i],
                            _input.filter->outputs[i].readPtr,
                            accLen,
                            _input.flowState, &returnFlowState
                    );

                // TODO: WTF to do with returnFlowState
                // We never get it in the multi-threaded case.

                DASSERT(lenConsumed <= accLen, "ring buffer read overrun");

                if(lenConsumed)
                    AdvanceReadPtr(output, lenConsumed);

                // TODO: FIX THIS flowState stuff.
                if(returnFlowState)
                    _input.flowState = returnFlowState;
            }
        }
    }
}


void qsOutput(size_t len, uint32_t outputChannelNum) {

    // The user API can't output 0 bytes.
    //
    DASSERT(len, "");

    _qsOutput(len, outputChannelNum);
}


//
void qsAdvanceInput(size_t len) {

    // We cannot let the filter module corrupt the circular buffer.
    ASSERT(len <= _input.len, "");

    // We cannot necessarily advance the output->readPtr because only the
    // thread of the filter that owns the output can do that.  Remember
    // there is a lock-less circular buffer.  We just need to keep a
    // record of how much was consumed and let the thread of the filter
    // that owns the output advance the readPtr later.  There's not a lot
    // of waste in doing it this way even when there are not multiple
    // threads.

    _input.len -= len;
    _input.advanceInput_wasCalled = true;
}


// This looping search is only called at in a filters start() which is
// only called at qsSourceReady(); so this is not called often.
//
static inline
struct QsOutput *FindOuputToFilter(struct QsStream *s,
        struct QsFilter *f, uint32_t inputNum) {

    for(uint32_t i=s->numConnections-1; i!=-1; --i) {
        if(s->to[i] == f) {
DSPEW("\"%s\" -> \"%s\"", s->from[i]->name, f->name);
            for(uint32_t j=s->from[i]->numOutputs-1; j!=-1; --j) {
DSPEW("inputChannelNum=%" PRIu32, s->from[i]->outputs[j].inputChannelNum);
                if(s->from[i]->outputs[j].inputChannelNum == inputNum) {
                    DASSERT(s->from[i]->outputs[j].filter == f, "");
                    return &(s->from[i]->outputs[j]);
                }
            }
        }
    }

    ASSERT(false, "filter \"%s\" input channel "
            "%" PRIu32 " output not found",
            f->name, inputNum);

    return 0; // stop compiler warning
}


static
void SetReadParameter(size_t len, uint32_t *inputNums, struct QsFilter *f,
        void (*callback)(struct QsOutput *output, size_t len)) {

    DASSERT(f,"");
    DASSERT(f->stream, "");

    if(inputNums == QS_ALLCHANNELS) {
        // This should have been checked in stream.c so we just check on
        // DEBUG now.
        DASSERT(f->numInputs < QS_MAX_CHANNELS, "");

        inputNums = alloca(sizeof(*inputNums)*(f->numInputs+1));
        inputNums[f->numInputs] = QS_ARRAYTERM;
        for(uint32_t i = f->numInputs - 1; i!=-1; --i) {
            inputNums[i] = i;
        }
    }

    for(; *inputNums != QS_ARRAYTERM; ++inputNums)
        // Find the filter that outputs to this filter, f.
        callback(FindOuputToFilter(f->stream, f, *inputNums), len);
}


// Instead of repeating this code 3 times we Macro function it.
//
#define SET_READ_PARAMETER(parameter, output, len)\
    do {\
        DSPEW("setting filter \"%s\" input %"\
            PRIu32 " " #parameter " to %zu",\
            (output)->filter->name, (output)->inputChannelNum, (len));\
        (output)->parameter = (len);\
    } while(0)


static
void SetMaxReadThreshold(struct QsOutput *output, size_t len) {
    DASSERT(len, "maxReadThreshold cannot be 0");
    SET_READ_PARAMETER(maxReadThreshold, output, len);
    
}


void qsSetMaxReadThreshold(size_t len, uint32_t *inputNums) {
    SetReadParameter(len, inputNums, _qsCurrentFilter, SetMaxReadThreshold);
}


static
void SetMinReadThreshold(struct QsOutput *output, size_t len) {
    SET_READ_PARAMETER(minReadThreshold, output, len);
}


void qsSetMinReadThreshold(size_t len, uint32_t *inputNums) {
    SetReadParameter(len, inputNums, _qsCurrentFilter, SetMinReadThreshold);
}


static
void SetMaxRead(struct QsOutput *output, size_t len) {
    SET_READ_PARAMETER(maxRead, output, len);
}


void qsSetMaxRead(size_t len, uint32_t *inputNums) {
    SetReadParameter(len, inputNums, _qsCurrentFilter, SetMaxRead);
}
