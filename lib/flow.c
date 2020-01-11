#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <pthread.h>

#include "./qs.h"
#include "../include/quickstream/filter.h"
#include "./debug.h"



// 1. Transfer job from the filter->queue to the stream job queue, then
//
// 2. transfer a job from filter unused to filter queue (to replace the
// one you just moved), and then
//
// 3. clean the filter queue job args.
//
// We must have a stream->mutex lock to call this.
static inline
void FilterQToStreamQAndSoOn(struct QsStream *s, struct QsJob *j) {

    DASSERT(s);
    DASSERT(j->next == 0);
    struct QsFilter *f = j->filter;
    DASSERT(f);

    /////////////////////////////////////////////////////////////////////
    // 1. Transfer job from the filter->queue to the stream job queue.

    if(s->jobLast) {
        // There are jobs in the stream queue.
        DASSERT(s->jobLast->next == 0);
        DASSERT(s->jobFirst);
        s->jobLast->next = j;
        s->jobLast = j;
    } else {
        // There are no jobs in the stream queue.
        DASSERT(s->jobFirst == 0);
        s->jobLast = s->jobFirst = j;
    }
    // We are done with j.  We can reuse it.

    /////////////////////////////////////////////////////////////////////
    // 2. Now transfer a job from filter unused to filter queue.

    DASSERT(f->unused);
    j = f->unused;
    f->unused = j->next;


    /////////////////////////////////////////////////////////////////////
    // 3. Now clean/reset the filter queue job args.

    if(f->numInputs) {
        // This is not a source filter.
        DASSERT(j->inputBuffers);
        DASSERT(j->inputLens);
        DASSERT(j->isFlushing);
        DASSERT(j->advanceLens);

        memset(j->inputBuffers, 0, f->numInputs*sizeof(*j->inputBuffers));
        memset(j->inputLens, 0, f->numInputs*sizeof(*j->inputLens));
        memset(j->isFlushing, 0, f->numInputs*sizeof(*j->isFlushing));
        memset(j->advanceLens, 0, f->numInputs*sizeof(*j->advanceLens));
    }
#ifdef DEBUG
    else {
        // It must be a source filter.  There is nothing to clean/reset.
        DASSERT(j->inputBuffers == 0);
        DASSERT(j->inputLens == 0);
        DASSERT(j->isFlushing == 0);
        DASSERT(j->advanceLens == 0);
    }
#endif
}


// This is called by the worker threads.
//
// Transfer job from the stream job queue to the worker thread function
// call stack.
//
// We must have a stream->mutex lock to call this.
static inline
struct QsJob *StreamQToWorker(struct QsStream *s) {

    DASSERT(s);

    if(s->jobFirst) {
        struct QsJob *j = s->jobFirst;
        DASSERT(s->jobLast);
        DASSERT(s->jobLast->next == 0);
        s->jobFirst = j->next;
        if(s->jobFirst == 0) {
            DASSERT(s->jobLast == j);
            s->jobLast = 0;
            // There are now no jobs in the stream job queue.
        } else {
            // There are still some jobs in the stream job queue.
            DASSERT(j->next);
            j->next = 0;
        }
        return j;
    }
    return 0;
}


// This is called by the worker threads.
//
// Transfer job from the worker thread function call stack to the filter
// unused stack.
//
// We must have a stream->mutex lock to call this.
static inline
void WorkerToFilterUnused(struct QsJob *j) {

    DASSERT(j->next == 0);
    struct QsFilter *f = j->filter;
    DASSERT(f);

    if(f->unused)
        j->next = f->unused;
    else
        f->unused = j;

    // The job args will be cleaned up later in FilterUnusedToFilterQ().
}


//static inline
void CheckLockOutput(struct QsFilter *f) {

    DASSERT(f);
    if(f->mutex)
        CHECK(pthread_mutex_lock(f->mutex));
}


//static inline
void CheckUnlockOutput(struct QsFilter *f) {

    DASSERT(f);
    if(f->mutex)
        CHECK(pthread_mutex_unlock(f->mutex));
}



// This is called by worker threads.
//
static
void *RunningWorkerThread(struct QsStream *s) {

    DASSERT(s);
    
    // The life of a worker thread.
    //
    bool living = true;

    // STREAM LOCK
    CHECK(pthread_mutex_lock(&s->mutex));

    // We work until we die.
    //
    while(living) {

        // Get the next job from the stream job queue.
        //
        struct QsJob *j = StreamQToWorker(s);


        if(j == 0) {
            // We are unemployed.  We have no job.  Just like I'll be
            // after I finish writing this fucking code.

            // We count ourselves in the ranks of the unemployed.
            ++s->numIdleThreads;

            DSPEW("Thread waiting for work");

            // STREAM UNLOCK
            // wait
            CHECK(pthread_cond_wait(&s->cond, &s->mutex));
            // STREAM LOCK

            // Remove ourselves from the numIdleThreads.
            --s->numIdleThreads;

            // Because there can be more than one thread woken by a
            // signal, we may still not have a job available for this
            // worker.  Such is life for the masses.
            continue;
        }

        // This worker has a new job.
        //
        // This thread can now read and write to this job without a mutex.
        // No other thread can access this job while it is not in a stream
        // queue, filter queue, or filter unused stack list.

        // Put it in this thread specific data so we can find it in the
        // filter when this thread runs things like qsAdvanceInput(),
        // qsOutput(), and other quickstream/filter.h functions.

        CHECK(pthread_setspecific(_qsKey, j));
        struct QsFilter *f = j->filter;
        DASSERT(f->numThreads < f->maxThreads);

        // Set the thread counters for this filter.  Have a stream mutex
        // lock for this:
        ++f->numThreads;

        // The next thread number is always increasing. Have a stream
        // mutex lock for reading and changing f->nextThreadNum.
        j->threadNum = f->nextThreadNum++;

        // j->threadNum is now letting us know the order of the thread
        // calling this filter input().  j->threadNum is not really needed
        // if this filter input does not support multiple threads.

        // STREAM UNLOCK
        CHECK(pthread_mutex_unlock(&s->mutex));

        int ret = f->input(j->inputBuffers, j->inputLens,
                j->isFlushing, f->numInputs, f->numOutputs);


        // Push jobs to the stream queue.



        if(ret) {
            if(ret < 0)
                WARN("filter \"%s\" input() returned error code %d",
                        f->name, ret);
                

        }


        // STREAM LOCK
        CHECK(pthread_mutex_lock(&s->mutex));

        // Unset the thread counters for this filter.  This worker no
        // longer has the job we just finished.
        --f->numThreads;


        // Move this job structure to the filter unused stack.
        WorkerToFilterUnused(j);
    }


    // STREAM UNLOCK
    CHECK(pthread_mutex_unlock(&s->mutex));


    return 0; // We're dead now.  It was a good life.
}




// Get a worker thread to do our thread input() call.
//
static inline
void FeedJobToWorkerThread(struct QsStream *s, struct QsFilter *f) {

    DASSERT(s);
    DASSERT(f);


    /////////////////////////////////////////////////////////////////////
    /////////////////// HAVE STREAM MUTEX ///////////////////////////////
    /////////////////////////////////////////////////////////////////////
    CHECK(pthread_mutex_lock(&s->mutex));

    // We have a job in the filter queue.
    DASSERT(f->queue);
    DASSERT(f->numThreads <= f->maxThreads);

    // Put the job in the queue that the workers get the jobs from.
    FilterQToStreamQAndSoOn(s, f->queue);


    if(s->numThreads != s->maxThreads) {
        //
        // Filter does not have its' quota of worker threads.
        //
        pthread_t thread;
        CHECK(pthread_create(&thread, 0/*attr*/,
                    (void *(*) (void *)) RunningWorkerThread, s));

        ++s->numThreads;

        // The new thread may be born and get its' job or another worker
        // thread may finish its' current job and take this job before
        // that new thread can.  So this thread may end up idle.  It's
        // okay either way.

    } else if(s->numIdleThreads) {
        //
        // There is a thread calling pthread_cond_wait(s->cond, s->mutex),
        // the s->mutex lock before that guarantees it.
        //
        CHECK(pthread_cond_signal(&s->cond));

        // At least one (and maybe more) worker threads will wake up
        // sometime after we release the s->mutex.  The threads that wake
        // up will handle the numIdleThreads counting.  We are done.
    }

    // else: a worker thread will get the job from the stream queue later.
    // We have no idle threads (that's good) and; we are running as many
    // worker threads as we can.  That may be good too, but running many
    // threads can cause contention, and the more threads the more
    // contention.  Performance measures can determine the optimum number
    // of threads.  Also we need to see how this flow runner method
    // (quickstream) compares to other streaming frame-works.


    CHECK(pthread_mutex_unlock(&s->mutex));
    /////////////////////////////////////////////////////////////////////
    /////////////////// RELEASED STREAM MUTEX ///////////////////////////
    /////////////////////////////////////////////////////////////////////

}


// Note: This call blocks.
static
uint32_t nThreadFlow(struct QsStream *s) {

    for(uint32_t i=0; i<s->numSources; ++i)
        FeedJobToWorkerThread(s, s->sources[i]);

    // Now the worker threads will run with the stream.


    DASSERT(s->masterWaiting == 0);



    return 0; // success
}


// Allocate the input arguments or so called job arguments.
static inline
void AllocateJobArgs(struct QsJob *job, uint32_t numInputs) {

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


    uint32_t numJobs = f->maxThreads + 1;
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

        AllocateJobArgs(f->jobs + i, numInputs);
        // Initialize the unused job stack:
        if(i >= 2)
            // Note: the top of f->jobs[] is used as the queue.  It's just
            // how we choice to initialize this data.
            (f->jobs + i - 1)->next = f->jobs + i;
    }
 
    // Initialize the job queue with one empty job.
    f->queue = f->jobs;
    // Set the top of the unused job stack.
    f->unused = f->jobs + 1;

    // Am I a stupid-head?
    DASSERT(f->jobs->next == 0);
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

    if(s->maxThreads == 0)
        // qsStreamLaunch() was not configured to run with worker
        // threads.
        return -1;

    if(s->numThreads == 0)
        // The number of worker threads is 0 and so there is no reason to
        // wait, and no worker threads to signal this main/master thread.
        return 1;


    CHECK(pthread_mutex_lock(&s->mutex));

    DASSERT(s->masterWaiting == false);

    s->masterWaiting = true;
    // The last worker to quit will signal this conditional.
    CHECK(pthread_cond_wait(&s->masterCond, &s->mutex));
    s->masterWaiting = false;
    CHECK(pthread_mutex_unlock(&s->mutex));

    return 0; // yes we did wait.
}


int qsStreamLaunch(struct QsStream *s, uint32_t maxThreads) {

    DASSERT(_qsMainThread == pthread_self(), "Not main thread");

    ASSERT(maxThreads!=0, "Write the code for the maxThread=0 case");
    ASSERT(maxThreads <= _QS_STREAM_MAXMAXTHREADS,
            "maxThread=%" PRIu32 " is too large (> %" PRIu32 ")",
            maxThreads, _QS_STREAM_MAXMAXTHREADS);

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

        // Set a stream flow function.
        s->flow = nThreadFlow;

        CHECK(pthread_mutex_init(&s->mutex, 0));
        CHECK(pthread_cond_init(&s->cond, 0));
        CHECK(pthread_cond_init(&s->masterCond, 0));

        StreamSetFilterMarks(s, true);
        for(uint32_t i=0; i<s->numSources; ++i)
            AllocateFilterJobsAndMutex(s, s->sources[i]);
    }


    ASSERT(s->flow, "Did not set a stream flow function. "
            "Write this code");

    return s->flow(s);
}
