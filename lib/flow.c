#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <pthread.h>

#include "./qs.h"
#include "../include/qsfilter.h"
#include "./debug.h"


#if 0

static void *StartThread(struct QsFilter *f) {

    DASSERT(f, "");
    DASSERT(f->app, "");

    // This data will exist so long as this thread uses this call stack.
    struct QsThreadData threadData;
    memset(&threadData, 0, sizeof(threadData));

    CHECK(pthread_setspecific(f->app->key, &threadData));

    return 0;
}

static void *FinishThread(void *ptr) {


    return 0;
}


static void CreateThread(struct QsStream *s, struct QsFilter *f) {

    pthread_t thread;

    CHECK(pthread_create(&thread, 0,
                (void *(*) (void *)) StartThread, f));


}

#endif



uint32_t nThreadFlow(struct QsStream *s) {

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


int qsStreamLaunch(struct QsStream *s, uint32_t maxThreads) {

    ASSERT(maxThreads!=0, "Write the code for the maxThread=0 case");
    ASSERT(maxThreads <= _QS_STREAM_MAXMAXTHTREADS,
            "maxThread=%" PRIu32 " is too large (> %" PRIu32 ")",
            maxThreads, _QS_STREAM_MAXMAXTHTREADS);

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    DASSERT(s, "");
    DASSERT(s->app, "");
    ASSERT(s->sources, "qsStreamReady() must be successfully"
            " called before this");
    ASSERT(!(s->flags & _QS_STREAM_LAUNCHED),
            "Stream has been launched already");

    DASSERT(s->maxInputPorts, "");
    DASSERT(s->maxInputPorts <= _QS_MAX_CHANNELS, "");


    DASSERT(s->numSources, "");

    s->flags |= _QS_STREAM_LAUNCHED;

    s->maxThreads = maxThreads;

    if(s->maxThreads) {

        CHECK(pthread_cond_init(&s->cond, 0));
        CHECK(pthread_mutex_init(&s->mutex, 0));

        s->jobs = calloc(s->maxThreads, sizeof(*s->jobs));
        ASSERT(s->jobs, "calloc(s->maxThreads,%zu) failed",
                sizeof(*s->jobs));
        for(uint32_t i=0; i<s->maxThreads; ++i) {
            struct QsJob *job = s->jobs + i;
            CHECK(pthread_mutex_init(&job->mutex, 0));
            CHECK(pthread_cond_init(&job->cond, 0));

            job->buffers = calloc(s->maxInputPorts, sizeof(*job->buffers));
            ASSERT(


        }
    }

    DASSERT(s->flow, "");


    return s->flow(s);
}
