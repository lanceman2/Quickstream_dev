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

    DASSERT(f);
    DASSERT(f->app);

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
        DASSERT(s->sources[i]);
        struct QsFilter *f = s->sources[i];
        DASSERT(f);
        DASSERT(f->input);



    }

    return 0; // success
}


// This recurses.
//
// This is not called unless s->maxThreads is non-zero.
static
void AllocateFilterJobs(struct QsFilter *f) {

    DASSERT(f);
    DASSERT(f->mark);
    DASSERT(f->maxThreads, "maxThreads cannot be 0");
    DASSERT(f->jobs == 0);
    DASSERT(f->mutex == 0);

    // Un-mark this filter.
    f->mark = false;

    f->mutex = malloc(sizeof(*f->mutex));
    ASSERT(f->mutex, "malloc(%zu) failed", sizeof(*f->mutex));
    CHECK(pthread_mutex_init(f->mutex, 0));


    if(f->numInputs) {

        uint32_t numJobs = f->maxThreads + 1;
        uint32_t numInputs = f->numInputs;

        f->jobs = calloc(numJobs, sizeof(*f->jobs));
        ASSERT(f->jobs, "calloc(%" PRIu32 ",%zu) failed",
                numJobs, sizeof(*f->jobs));

        for(uint32_t i=0; i<numJobs; ++i) {
            struct QsJob *job = f->jobs + i;

            job->buffers = calloc(numInputs, sizeof(*job->buffers));
            ASSERT(job->buffers, "calloc(%" PRIu32 ",%zu) failed",
                    numInputs, sizeof(*job->buffers));
            job->lens = calloc(numInputs, sizeof(*job->lens));
            ASSERT(job->lens, "calloc(%" PRIu32 ",%zu) failed",
                    numInputs, sizeof(*job->lens));
            job->isFlushing = calloc(numInputs, sizeof(*job->isFlushing));
            ASSERT(job->isFlushing, "calloc(%" PRIu32 ",%zu) failed",
                    numInputs, sizeof(*job->isFlushing));

            // Initialize the unused job stack:
            if(i)
                (job - 1)->next = job;
            else
                f->unused = job;
        }
        // Initialize the job queue:
        f->queue = 0;
    }

    // Recurse if we need to.
    for(uint32_t i=0; i<f->numOutputs; ++i) {
        struct QsOutput *output = f->outputs + i;
        for(uint32_t j=0; j<output->numReaders; ++j) {
            struct QsFilter *nextFilter = (output->readers + j)->filter;
            DASSERT(nextFilter);
            if(nextFilter->mark)
                AllocateFilterJobs(nextFilter);
        }
    }
}


int qsStreamLaunch(struct QsStream *s, uint32_t maxThreads) {

    ASSERT(maxThreads!=0, "Write the code for the maxThread=0 case");
    ASSERT(maxThreads <= _QS_STREAM_MAXMAXTHTREADS,
            "maxThread=%" PRIu32 " is too large (> %" PRIu32 ")",
            maxThreads, _QS_STREAM_MAXMAXTHTREADS);

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    DASSERT(s);
    DASSERT(s->app);
    ASSERT(s->sources, "qsStreamReady() must be successfully"
            " called before this");
    ASSERT(!(s->flags & _QS_STREAM_LAUNCHED),
            "Stream has been launched already");

    DASSERT(s->numSources);

    s->flags |= _QS_STREAM_LAUNCHED;

    s->maxThreads = maxThreads;


    if(s->maxThreads) {

        CHECK(pthread_cond_init(&s->cond, 0));
        CHECK(pthread_mutex_init(&s->mutex, 0));

        StreamSetFilterMarks(s, true);
        for(uint32_t i=0; i<s->numSources; ++i)
            AllocateFilterJobs(s->sources[i]);
    }

    // There is a stream flow function.
    DASSERT(s->flow);

    return s->flow(s);
}
