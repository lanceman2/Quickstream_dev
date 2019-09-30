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
sendOutput_sameThread(struct QsOutput *output, uint32_t inputChannelNum,
        uint32_t flowState, uint32_t *returnFlowState) {

    DASSERT(output, "");
    DASSERT(output->filter, "");


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



