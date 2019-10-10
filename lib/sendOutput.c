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
static size_t Input(struct QsOutput *output, uint8_t *buf, size_t totalLen,
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

    _input.filter = filter;
 
    do {
        size_t len = remainingLen;

        if(output->maxReadSize && len > output->maxReadSize)
            len = output->maxReadSize;

        // _input is the threads object state holder that is accessed in
        // qsAdvanceInput(), qsGetBuffer(), and qsOutput(), which may be
        // called in filter->input().
        //
        _input.len = len;
        _input.advanceInput_wasCalled = false;
        _input.flowState = flowStateIn;

        int ret = filter->input(buf, len, output->inputChannelNum, flowStateIn);

        switch(ret) {
            case QsFContinue:
                break;
            case QsFFinished:
                flowStateIn |= _QS_LASTPACKAGE;
                break;
            default:
                WARN("filter \"%s\" input() returned "
                    "unknown enum QsFilterInputReturn %d",
                    filter->name, ret);
        }

        if(!_input.advanceInput_wasCalled)
            // By default we advance the buffer all the way.
            qsAdvanceInput(len);

        DASSERT(len >= _input.len, "");

        // Advance the buffer and find the remaining length.
        size_t dif = len - _input.len;
        buf += dif;
        remainingLen -= dif;

    } while(remainingLen >= output->maxReadThreshold);


    // Get this value returned to the calling function.
    *flowStateReturn = flowStateIn;

    return totalLen - remainingLen;
}



// Case same thread, not a source filter.
//
static size_t
sendOutput_sameThread(struct QsFilter *filter,
        struct QsOutput *output, uint8_t *buf, size_t totalLen,
        uint32_t flowStateIn, uint32_t *flowStateReturn) {

    DASSERT(output, "");

    // Note: in this case we can pass the flowStateReturn directly to the
    // filter Input() wrapper, but if it was in another thread, there's a
    // lot more to getting values to and back from Input().

    // Save _input state on the stack in this function call.
    //
    // TODO: We could have passed this data as an argument to Input()
    // and that would have the same effect, it would be an argument on the
    // stack and not just a static definition on the stack in this
    // particular call of this function.
    //
    // TODO: This is a waste of time if there will be no more Input()
    // calls added to this stack.  But predicting filter input() calls may
    // not be that easy.
    //
    struct QsInput saveInputState;
    memcpy(&saveInputState, &_input, sizeof(saveInputState));

    // put another filter input() call on the stack.
    size_t ret = Input(output, buf, totalLen, flowStateIn, flowStateReturn);

    // Now the variable _input is changed from other calls to other filter
    // input() calls.
    //
    // Restore _input state from stack saveInputState because the Input()
    // call may have called a stack of Input() -> input() calls.  This
    // will let the filters input() call that is currently on the stack be
    // able to call  qsOutput(), qsAdvanceInput(), and qsGetBuffer().
    //
    memcpy(&_input, &saveInputState, sizeof(saveInputState));

    return ret;
}
//
// This is only called for filter that is a source filter.
// The output is not from another filter so output=0.
static size_t
sendOutput_sameThread_source(struct QsFilter *filter,
        struct QsOutput *output, uint8_t *buf, size_t totalLen,
        uint32_t flowStateIn, uint32_t *flowStateReturn) {

    DASSERT(buf == 0, "");
    DASSERT(totalLen == 0, "");
    DASSERT(output == 0, "");

    _input.filter = filter;
    _input.len = 0;
    _input.advanceInput_wasCalled = false;
    _input.flowState = flowStateIn;

    // For this case we do not need to store _input state like in
    // sendOutput_sameThread().

    // Input 0 bytes on channel 0 to a source filter.
    int ret = filter->input(0, 0, 0, flowStateIn);

    // At this point there are no filter input() calls on the stack.
    // Ya, single threaded.

    switch(ret) {
        case QsFContinue:
            break;
        case QsFFinished:
            flowStateIn |= _QS_LASTPACKAGE;
            break;
        default:
            WARN("filter \"%s\" input() returned "
                    "unknown enum QsFilterInputReturn %d",
                    filter->name, ret);
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

