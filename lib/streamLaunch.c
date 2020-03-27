#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>

#include "./debug.h"
#include "./qs.h"
#include "./flowJobLists.h"
#include "../include/quickstream/filter.h"
#include "../include/quickstream/app.h"




// This function starts the flow by pouring threads into all the source
// filters.
static
uint32_t nThreadFlow(struct QsStream *s) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    DASSERT(s);

    // Set all filter->mark = false as in not finishing flowing, or
    // calling input().
    //
    // filter->mark will be true when the filter is finished.
    // Filters must finish in the order of the filter graph flow.
    //
    // A Filter is flushing when the filter feeding the particular channel
    // is finished.  The filter reads a isFlushing input port when the
    // feeding filter will no longer have input() called.
    StreamSetFilterMarks(s, false);


    // LOCK stream mutex
    CHECK(pthread_mutex_lock(&s->mutex));

    // 0. set isSourcing
    //
    s->isSourcing = 1;

    // 1. Now add source filter jobs to the stream queue.
    //
    // TODO: We need to consider adding a functionality that wraps
    // epoll_wait(2) to add a different way to add jobs for filters that
    // can call would be blocking read(2) or write(2) (or like system
    // call) to the stream queue that is triggered by epoll events from
    // file descriptors that that are registered for a given filter.  For
    // filter streams with many sources and sinks that read and write file
    // descriptors this could increase performance considerably.  It
    // could get rid of a lot of threads that are just waiting on blocked
    // system calls.
    //
    // TODO: If a source filter is multi-threaded we should loop again and
    // again until there the source filters can have their fill of
    // threads.
    //
    for(uint32_t i=0; i<s->numSources; ++i)
        FilterUnusedToStreamQ(s, s->sources[i]);

    // 2. Now launch as many threads as we can up to the number of source
    //    filters.  More threads may get added later, if there is demand;
    //    or threads could be removed if they are too idle, or too
    //    contentious; (TODO) if we figured out how to write that kind of
    //    code.
    //
    //    If there are fewer threads than there are source filters that's
    //    fine.  The jobs are queued up, and will be worked on as worker
    //    threads finish their current jobs.
    //
    for(uint32_t i=0; i<s->numSources &&
            s->numThreads < s->maxThreads &&
            s->maxThreads; ++i)
        LaunchWorkerThread(s);


    // All starting threads will just wait on getting this stream mutex
    // lock.  So they cannot possibly do anything but wait for a mutex
    // lock until we unlock the stream mutex below.

    // UNLOCK stream mutex
    CHECK(pthread_mutex_unlock(&s->mutex));

    // Now the worker threads will run wild in the stream.

    DASSERT(s->masterWaiting != true);

    // 3. If this has no worker threads, i.e. the workers are all on
    // strike, then we run it with this one main thread.  This main thread
    // will be the one worker thread.

    if(s->maxThreads == 0) {

        // See LaunchWorkerThread() in qs.h.
        // We pretend we are a worker by saying that maxThreads = 1.
        // Otherwise the code will shit itself.
        s->maxThreads = 1;
        DASSERT(s->numThreads == 0);
        s->numThreads = 1;

        // This next call may take a while.  Here's where management
        // goes to work. 
        RunningWorkerThread(s);

        // Just in case this state needs to be known.
        s->maxThreads = 0; // We lied.  maxThread really is 0.
        s->numThreads = 0;
        // The regular worker threads would have had this set as they
        // worked, but this is now again acting like the master thread
        // and the master thread does not setup jobs in it's
        // pthread_setspecific data under this key _qsKey.
        CHECK(pthread_setspecific(_qsKey, 0));
    }

    return 0; // success
}


// Allocate the input arguments or so called job arguments.
static inline
void AllocateJobArgs(struct QsFilter *f, struct QsJob *job,
        uint32_t numInputs, uint32_t numOutputs) {

    if(numOutputs) {
        job->outputLens = calloc(numOutputs,
                sizeof(*job->outputLens));
        ASSERT(job->outputLens, "calloc(%" PRIu32 ",%zu) failed",
                numOutputs, sizeof(*job->outputLens));
    }

    if(numInputs == 0) return;

    job->inputBuffers = calloc(numInputs, sizeof(*job->inputBuffers));
    ASSERT(job->inputBuffers, "calloc(%" PRIu32 ",%zu) failed",
            numInputs, sizeof(*job->inputBuffers));
    job->inputLens = calloc(numInputs, sizeof(*job->inputLens));
    ASSERT(job->inputLens, "calloc(%" PRIu32 ",%zu) failed",
            numInputs, sizeof(*job->inputLens));
    job->isFlushing = calloc(numInputs, sizeof(*job->isFlushing));
    ASSERT(job->isFlushing, "calloc(%" PRIu32 ",%zu) failed",
            numInputs, sizeof(*job->isFlushing));
    job->advanceLens = calloc(numInputs, sizeof(*job->advanceLens));
    ASSERT(job->advanceLens, "calloc(%" PRIu32 ",%zu) failed",
            numInputs, sizeof(*job->advanceLens));
}


// This recurses.
//
// This is not called unless s->maxThreads is non-zero.
static
void AllocateFilterJobsAndMutex(struct QsStream *s, struct QsFilter *f) {

    DASSERT(f);
    DASSERT(f->mark);
    DASSERT(f->maxThreads, "maxThreads cannot be 0");
    DASSERT(f->jobs == 0);

    // Un-mark this filter.
    f->mark = false;

    uint32_t numJobs = GetNumAllocJobsForFilter(s, f);
    uint32_t numInputs = f->numInputs;

    f->jobs = calloc(numJobs, sizeof(*f->jobs));
    ASSERT(f->jobs, "calloc(%" PRIu32 ",%zu) failed",
            numJobs, sizeof(*f->jobs));

    DASSERT(f->mutex == 0);
    DASSERT(f->maxThreads != 0);

    if(s->maxThreads > 1 && f->maxThreads > 1) {
        f->mutex = malloc(sizeof(*f->mutex));
        ASSERT(f->mutex, "malloc(%zu) failed", sizeof(*f->mutex));
        CHECK(pthread_mutex_init(f->mutex, 0));
        // Not lock-less buffers, but we have multi-threaded filter
        // input().
    }
    // else: We have lock-less buffers.


    for(uint32_t i=0; i<numJobs; ++i) {

        f->jobs[i].filter = f;
#ifdef DEBUG
        f->jobs[i].magic = _QS_IS_JOB;
#endif

        AllocateJobArgs(f, f->jobs + i, numInputs, f->numOutputs);
        // Initialize the unused job stack:
        // All the jobs start in the unused stack.
        if(i >= 1)
            (f->jobs + i - 1)->next = f->jobs + i;
    }
 
    // Set the top of the unused job stack.
    f->unused = f->jobs;

    // Am I a stupid-head?
    DASSERT(f->jobs->next || numJobs == 1);
    DASSERT((f->jobs+numJobs-1)->next == 0);


    // Recurse if we need to.
    for(uint32_t i=0; i<f->numOutputs; ++i) {
        struct QsOutput *output = f->outputs + i;
        for(uint32_t j=0; j<output->numReaders; ++j) {
            struct QsFilter *nextFilter = (output->readers + j)->filter;
            DASSERT(nextFilter);
            if(nextFilter->mark)
                AllocateFilterJobsAndMutex(s, nextFilter);
        }
    }
}


int qsStreamWait(struct QsStream *s) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");
    ASSERT(s->sources, "qsStreamReady() must be successfully"
            " called before this");
    ASSERT(s->flags & _QS_STREAM_LAUNCHED,
            "Stream has not been launched");

    // LOCK stream mutex
    CHECK(pthread_mutex_lock(&s->mutex));

    if(s->numThreads == 0) {
        // The number of worker threads is 0 and so there is no reason to
        // wait, and no worker threads to signal this main/master thread.
        //
        // UNLOCK stream mutex
        CHECK(pthread_mutex_unlock(&s->mutex));
        return 1;
    }


    DASSERT(s->masterWaiting == false);

    s->masterWaiting = true;

    // The last worker to quit will signal this conditional.
    //
    // unlock stream mutex via pthread_cond_wait()
    // 
    // wait via pthread_cond_wait()
    //
    CHECK(pthread_cond_wait(&s->masterCond, &s->mutex));
    //
    // now it's locked again via pthread_cond_wait()
    
    s->masterWaiting = false;

    // UNLOCK stream mutex
    CHECK(pthread_mutex_unlock(&s->mutex));

    return 0; // yes we did wait.
}


int qsStreamLaunch(struct QsStream *s, uint32_t maxThreads) {

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

    // TODO: remove pthreads synchronization calls in this code for the
    // case then s->maxThreads = 0 and s->maxThreads = 1.

    // Set a stream flow function.
    s->flow = nThreadFlow;

    CHECK(pthread_mutex_init(&s->mutex, 0));
    CHECK(pthread_cond_init(&s->cond, 0));
    CHECK(pthread_cond_init(&s->masterCond, 0));

    StreamSetFilterMarks(s, true);
    for(uint32_t i=0; i<s->numSources; ++i)
        AllocateFilterJobsAndMutex(s, s->sources[i]);

    return s->flow(s);
}


void qsStreamStopSources(struct QsStream *s) {

    // I guess we could have any thread (worker or not)  call this.
    //DASSERT(_qsMainThread == pthread_self(), "Not main thread");

    DASSERT(s);
    // We assuming with is not called 2 billion times so this atomic
    // counter does not wrap through zero in a single flow cycle.
    //
    // Note we do not use a mutex to change this atomic variable so that
    // this can be used in a signal handler.
    //
    // atomic variable change:
    DSPEW();
    --s->isSourcing;
}
