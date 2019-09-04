#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/mman.h>

#include "./qsfilter.h"
#include "./qsapp.h"
#include "./debug.h"


__thread struct QsFilter *_qsCurrentFilter = 0;





static inline
void _AllocateRingBuffer(struct QsOutput *o) {

    DASSERT(o, "");



}


// Here we are setting up the memory conveyor belt that connects filters
// in the stream.  Note this recurses.
//
void AllocateRingBuffers(struct QsFilter *f) {

    DASSERT(f, "");

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



}



// The current writer filter gets an output buffer.
void *qsBufferGet(size_t len, uint32_t outputChannelNum) {

    DASSERT(_qsCurrentFilter,"");

    return 0;
}


void qsOutput(size_t len, uint32_t outputChannelNum) {

    DASSERT(_qsCurrentFilter,"");


}

