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






void _AllocateOutputBuffer(struct QsOutput *o) {

    DASSERT(o, "");



}


// Here we are setting up the memory conveyor belt that connects filters
// in the stream.  Note this recurses.
//
void AllocateOutputBuffers(struct QsFilter *f) {

    DASSERT(f, "");

    for(uint32_t i=0; i<f->numOutputs; ++i)
        _AllocateOutputBuffer(f->outputs+i);

    // Allocate for all children.
    for(uint32_t i=0; i<f->numOutputs; ++i)
        AllocateOutputBuffers(f->outputs[i].filter);


}


// Note this does not recurse.
//
void FreeOutputBuffers(struct QsFilter *f) {

    DASSERT(f, "");



}



// The current filter gets an output buffer.
void *qsBufferGet(size_t len, uint32_t outputChannelNum) {

    DASSERT(_qsCurrentFilter,"");

    return 0;
}


void qsPush(size_t len, uint32_t outputChannelNum) {

    DASSERT(_qsCurrentFilter,"");


}

