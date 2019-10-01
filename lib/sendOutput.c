#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The public installed user interfaces:
#include "../include/qsapp.h"
#include "../include/qsfilter.h"

// Private interfaces.
#include "./qs.h"
#include "./debug.h"


#if 0
static int Run(struct QsStream *s, struct QsFilter *f) {

    return 0;

}
#endif



// Case same thread
//
static size_t
sendOutput_sameThread(struct QsFilter *filter,
        struct QsOutput *output,
        uint32_t inputChannelNum) {

    DASSERT(filter, "");
    // Source filters do not get output from another filter, so they do
    // not have struct output set when their input() is called.
    DASSERT(!output || output->filter == filter, "");
    DASSERT((!output && inputChannelNum == 0) || output, "");

    struct QsStream *stream = filter->stream;
    DASSERT(stream, "");

    uint32_t returnFlowState = stream->flowState;
    
    // TODO: MORE HERE

    void *buf = 0;
    size_t len = 0;

    if(output) {
        HERE LANCE
    }

    // TODO: adjust len based on output reader parameters.

    // For this single thread case we do not need to pass the stream flow
    // state through the call state.
    //
    // TODO: Follow the flow state better...  It's likely a source filter
    // set the flow status so returning and popping the function call
    // stack, full of input() calls with the flow state flags set will
    // work fine, but if a non-source filter sets it, what is good than.

    int ret = filter->input(buf, len, inputChannelNum, stream->flowState);

    switch(ret) {
        case QsFContinue:
            break;
        case QsFFinished:
            stream->flowState |= _QS_LASTPACKAGE;
            break;
        default:
            WARN("filter \"%s\" input() returned "
                    "unknown enum QsFilterInputReturn %d",
                    s->sources[i]->name, returnFlowState);
    }


    return 0;
}


void setupSendOutput(struct QsFilter *f) {

    // This will work, even if there are loops in the stream flow.

    DASSERT(f, "");
    DASSERT(f->numOutputs, "");
    DASSERT(f->outputs, "");

    if(f->outputs[0].filter->sendOutput)
        // We already setup this sendOutput() for all outputs in this
        // filter, f.  We must have loops in the flow in this case.
        return;

    for(uint32_t i=0; i<f->numOutputs; ++i) {
        DASSERT(f->outputs[i].filter, "");
        DASSERT(f->outputs[i].filter->input, "");
        /////////////////////////////////////////////////////////////////
        // Filter input() will get called in the same thread and process.
        //
        f->outputs[i].filter->sendOutput = sendOutput_sameThread;
    }

    // At this point all the outputs for this filter, f, have a
    // sendOutput() so now we check the next output level.

    for(uint32_t i=0; i<f->numOutputs; ++i)
        if(f->outputs[i].filter->numOutputs)
            // Recurse.
            setupSendOutput(f->outputs[i].filter);
}


void unsetupSendOutput(struct QsFilter *f) {

    // This will work, even if there are loops in the stream flow.

    DASSERT(f, "");
    DASSERT(f->numOutputs, "");
    DASSERT(f->outputs, "");

    if(f->outputs[0].filter->sendOutput == 0)
        // We already setup this sendOutput() for all outputs in this
        // filter, f.  We must have loops in the flow in this case.
        return;

    for(uint32_t i=0; i<f->numOutputs; ++i) {
        DASSERT(f->outputs[i].filter, "");
        DASSERT(f->outputs[i].filter->input, "");
        f->outputs[i].filter->sendOutput = 0;
    }

    // At this point all the outputs for this filter, f, have a sendOutput()
    // zeroed, so now we check the next output level.

    for(uint32_t i=0; i<f->numOutputs; ++i)
        if(f->outputs[i].filter->numOutputs)
            // Recurse.
            unsetupSendOutput(f->outputs[i].filter);
}



