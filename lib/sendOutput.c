#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The public installed user interfaces:
#include "../include/qsapp.h"
#include "../include/qsfilter.h"

// Private interfaces.
#include "./qs.h"
#include "./debug.h"



//
// This is a wrapper that is called in the thread that is calling the
// filter module input().  The caller may have caused this to be called in
// the same thread, another thread/filter, or maybe another
// process/filter.
//
// output is from the feeding filter, we came only read stream run-time
// constant parts of it like inputChannelNum, maxReadThreshold,
// minReadThreshold, and maxReadSize; anything that needs to change in it
// can't be changed in this function.
//
// Arguments are passed in the stack in this thread that is calling this,
// but they may be gotten from and are pointers to inter-thread or
// inter-process shared memory.
//
static size_t Input(struct *output, uint8_t *buf, size_t totalLen,
        uint32_t flowStateIn, uint32_t *flowStateReturn) {

    // This Input() function runs in the thread that is calling the filter
    // input().
    //
    // buf and totalLen must have been computed in the feeding filter
    // thread.
    //

    DASSERT(output, "");
    DASSERT(totalLen, "");
    // We call this filter input()
    struct QsFilter *filter = output->filter;
    DASSERT(filter, "");
    // We Assume that the minimum threshold was met.
    DASSERT(totalLen >= output->minReadThreshold, "");

    // _input is the threads object state holder that is accessed in
    // qsAdvanceInput() which may be called in filter->input().
    //
    size_t remainingLen = totalLen;
 
    do {

        size_t len = remainingLen;

        if(len > output->maxReadSize)
            len = output->maxReadSize;

        // _input is the threads object state holder that is accessed in
        // qsAdvanceInput() which may be called in filter->input().
        //
        _input.len = len;
        _input.advanceInput_wasCalled = false;

        int ret = filter->input(buf, len, inputChannelNum, flowStateIn);

        switch(ret) {
            case QsFContinue:
                break;
            case QsFFinished:
                flowStateIn |= _QS_LASTPACKAGE;
                break;
            default:
                WARN("filter \"%s\" input() returned "
                    "unknown enum QsFilterInputReturn %d",
                    s->sources[i]->name, returnFlowState);
        }

        if(!_input.advanceInput_wasCalled)
            // By default we advance the buffer all the way.
            qsAdvanceInput(len);

        // Advance the buffer and find the remaining length.
        buf += _input.len;
        remainingLen -= (len - _input.len);

    } while(remainingLen >= output->maxReadThreshold);



    // Get this value returned to the calling function.
    *flowStateReturn = flowStateIn;

    return totalLen - remainingLen;
}



// Case same thread
//
static size_t
sendOutput_sameThread(struct QsFilter *filter,
        struct QsOutput *output, uint32_t inputChannelNum,
        uint32_t flowStateIn, uint32_t *flowStateReturn) {

    // Note: in this case we can pass the flowStateReturn directly to the
    // filter Input() wrapper, but if it was in another thread, there's a
    // lot more to getting that value back.

    return Input(filter, output, inputChannelNum,
            flowStateIn, flowStateReturn);
}
//
//
static size_t
sendOutput_sameThread_source(struct QsFilter *filter,
        struct QsOutput *output, uint32_t inputChannelNum,
        uint32_t flowStateIn, uint32_t *flowStateReturn) {

    // Input 0 bytes on channel 0 to a source filter.
    int ret = filter->input(0, 0, 0, flowStateIn);

    switch(ret) {
        case QsFContinue:
            break;
        case QsFFinished:
            flowStateIn |= _QS_LASTPACKAGE;
            break;
        default:
            WARN("filter \"%s\" input() returned "
                    "unknown enum QsFilterInputReturn %d",
                    s->sources[i]->name, returnFlowState);
    }

    *flowStateReturn = flowStateIn;

    return 0; // The sources consume 0 bytes.
}






void setupSendOutput(struct QsFilter *f) {

    // This will work, even if there are loops in the stream flow.

    DASSERT(f, "");
    DASSERT(f->numOutputs, "");
    DASSERT(f->outputs, "");

    if(f->u.numInputs == 0) {
        // This filter, f, is a source.
        f->sendOutput = sendOutput_sameThread_source;
    }


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



