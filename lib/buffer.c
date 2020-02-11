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

            struct QsOutput *feedOutput = output;
            // Find the top feeding output.
            while(feedOutput->prev) feedOutput = feedOutput->prev;
            // now o is points to the owner or top feed output.
            // This "pass through" buffer will use the feed buffer.
            DASSERT(feedOutput->buffer);
            output->buffer = feedOutput->buffer;
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
        DASSERT(b->end);
        DASSERT(b->mapLength);
        DASSERT(b->overhangLength);
#ifdef DEBUG
        // TODO: Is this really useful?
        memset(b->end - b->mapLength, 0, b->mapLength);
#endif
        // freeRingBuffer() calls munmap() ...
        freeRingBuffer(b->end - b->mapLength, b->mapLength,
                b->overhangLength);
#ifdef DEBUG
        memset(b, 0, sizeof(*b));
#endif
        free(b);
        output->buffer = 0;
    }
}


static inline
void GetMappingLengths(struct QsOutput *output, struct QsBuffer *buffer) {

    // This starts at the top of the output "pass-through" list:
    DASSERT(output->prev == 0);
    DASSERT(output->buffer);
    DASSERT(output->buffer == buffer);

    size_t mapLen = 0; // the bulk memory mapping length
    size_t overhangLen = 0; // overhanging memory mapping length

    // The overhang length is mapped on the end of the bulk mapping and
    // maps that end back to the start, making circular buffer.

    // This is the most important code in all of quickstream.  It defines
    // the size of the memory in the ring (circular) buffers.  The
    // buffering between filters is defined by the maximum read and write
    // promises made by the filters.  If the filters do not keep their
    // read and write promises the code will ASSERT() and stop running, in
    // place of over-running the memory in the buffers, which could happen
    // without the ASSERT().

    while(output) {

        // We refer to the level as the distance down the output list in
        // the linked list of "pass-through" outputs.
        //
        size_t levelLen = output->maxWrite;

        // Check the length of all read promises.
        for(uint32_t i=output->numReaders-1; i!=-1; --i)
            if(levelLen < output->readers[i].maxRead)
                levelLen = output->readers[i].maxRead;

        // levelLen is now the maximum length that will be written or
        // read at this output level.
        DASSERT(output->maxLength == 0);

        output->maxLength = levelLen;

        // grow the total length of the bulk mapping.
        mapLen += 2*levelLen;

        // The overhang mapping must be the maximum of any single write or
        // read operation for all filters that access this buffer.
        if(overhangLen < levelLen)
            overhangLen = levelLen;

        // Down the list we go.
        output = output->next;
    }

    buffer->mapLength = mapLen;
    buffer->overhangLength = overhangLen;
}


// TODO: go through the pass-through buffer list to tally the needed
// size.
static inline
void MakeRingBuffer(struct QsOutput *output) {

    DASSERT(output);
    struct QsBuffer *b = output->buffer;

    GetMappingLengths(output, b);

    // makeRingBuffer() will round up mapLength and
    // overhangLength to the nearest page.
    b->end = makeRingBuffer(&b->mapLength, &b->overhangLength);
    // makeRingBuffer() returns the start, we save this value in "end".
    b->end += b->mapLength;
    DSPEW("Made ring buffer bulk %zu with %zu overhang",
            b->mapLength, b->overhangLength);
}


// This function will recurse, i.e. call itself.
//
void MapRingBuffers(struct QsFilter *f) {

    f->mark = false;

    for(uint32_t i=0; i<f->numOutputs; ++i) {
        struct QsOutput *output = f->outputs + i;

        if(output->prev == 0) {
            // This is NOT a pass through buffer. 
            DASSERT(output->buffer);
            MakeRingBuffer(output);
        } else {
            // This is a pass through buffer that points to the "real"
            // buffer in output->prev.  It owns the allocated output
            // (QsOutput) data, but output->buffer is not owned by this
            // output.
            struct QsOutput *o = output->prev;
            // This is a "pass through" buffer so it does not have it's
            // own mapping or buffer struct.  We find the buffer that will
            // have the map memory by looking up the "pass through" buffer
            // list.
            while(o->prev) o = o->prev;
            // o is now the output with the buffer that owns the memory
            // for the buffer.  It's the original output that writes to
            // the buffer first in the series of pass through buffers that
            // share this buffers memory mapping.
            DASSERT(o->buffer);
            output->buffer = o->buffer;
        }

        // Initialize the writer
        output->writePtr = output->buffer->end - output->buffer->mapLength;

        DASSERT(output->numReaders);
        DASSERT(output->readers);

        for(uint32_t j=0; j<output->numReaders; ++j) {
            // Initialize the readers
            output->readers[j].readPtr = output->writePtr;
            output->readers[j].buffer = output->buffer;
        }
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
