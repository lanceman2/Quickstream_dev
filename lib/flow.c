#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>

#include "./qs.h"
#include "../include/qsfilter.h"
#include "./debug.h"



uint32_t singleThreadFlow(struct QsStream *s) {

    for(uint32_t i=0; i<s->numSources; ++i) {
        DASSERT(s->sources[i],"");
        struct QsFilter *f = s->sources[i];
        DASSERT(f,"");
        DASSERT(f->input, "");
#if 0
        // Get main thread specific data that we use in calls from
        // f->input() to buffer access functions like: qsGetBuffer(),
        // qsAdvanceInputs(), and qsOutputs().
        //
        struct QsInput *input = &s->app->input;
        size_t len[f->numOutputs];
        memset(input, 0, sizeof(*len)*f->numOutputs);
        input->filter = f;
        memset(len, 0, sizeof(*len)*f->numOutputs);
        input->len = len;

        int ret = f->input(0, 0, 0, f->numInputs, f->numOutputs);

        uint32_t isFlushing[f->numOutputs];
        memset(isFlushing, 0, sizeof(isFlushing));

        if(ret)
            for(uint32_t i=0; i<f->numOutputs; ++i)
                isFlushing[i] = true;

        DSPEW("f=%p isFlushing[0]=%d", input->filter, isFlushing[0]);
#endif
    }

    return 0; // success
}
