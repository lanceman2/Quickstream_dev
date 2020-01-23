#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <pthread.h>

#include "./debug.h"
#include "./qs.h"
#include "./flowJobLists.h"
#include "../include/quickstream/filter.h"



// Returns true if input() can be called more with the current input
// buffers.
//
// We must have the filter mutex lock (if multi-threaded filter) before
// calling this.
static inline
bool ProcessInput(struct QsFilter *f, struct QsJob *j,
        uint32_t numInputs) {

    bool keepInputing = false;
    bool allFlushing = true;

    // We need to check if we need to keep calling input() in this thread;
    // because sometimes the amount of data input into the filter is too
    // large for it to swallow, due to having smaller output buffers, or
    // limited processing capabilities that chock it.
    for(uint32_t i=0; i<numInputs; ++i) {
        if(j->inputLens[i] - j->advanceLens[i] >= f->readers[i]->maxRead) {
            // Call input until we cut inputLen less than maxRead
            keepInputing = true;
            break;
        }
        if(j->isFlushing[i] == false)
            // In this case this thread continues to call input() until
            allFlushing = false;
    }

    // We got to be careful if all input feeds to this filter have
    // finished, isFlushing=true for all feeds, then without the feeds the
    // input() needs to keep being called until input() returns non-zero
    // or all input data is gone.  Note: This extra loop is not accessed
    // often, this only happens at the end of the flow, at flushing
    // time.
    if(allFlushing && !keepInputing) {
        for(uint32_t i=0; i<numInputs; ++i)
            if(j->inputLens[i] > 0) {
                // There is some input data to read and we have no input
                // feeds (is Flushing), so we most keep calling input().
                keepInputing = true;
                break;
            }
    }


    // Now loop again knowing if we will be calling input() again.
    // We need to advance the read pointers and if input() is being called
    // again (if keepInputing) we need to fix the job arguments.
    for(uint32_t i=f->numInputs-1; i!=-1; --i) {

        // advanceLen is all we did read.
        size_t advanceLen = j->advanceLens[i];
        // inputLen is all we can read.
        size_t inputLen = j->inputLens[i];
        // This filter, f, owns this reader and can read/write
        // reader->readPtr.
        struct QsReader *reader = f->readers[i];

        DASSERT(reader->readLength >= inputLen);

        DASSERT(inputLen <= reader->buffer->mapLength);
        DASSERT(reader->readPtr <
                reader->buffer->mem + reader->buffer->mapLength);

        // Check for read overrun.  This should have been checked in
        // qsAdvanceInput().
        DASSERT(advanceLen <= inputLen,
                "Filter \"%s\" Buffer over read",
                f->name);

        // Check for read under-run.  Not enough reading that was promised
        // by the filter.  This is an error in the code of the filter
        // module.
        ASSERT(advanceLen > 0 || inputLen < reader->maxRead,
                // To bit hell they go.
                "Filter \"%s\" Buffer under read "
                "promise: maxRead=%zu <= input length %zu",
                f->name, reader->maxRead, inputLen);

        if(advanceLen) {
            // Advance readPtr.  This filter, f, owns this reader and can
            // read/write reader->readPtr.
            //
            // Note: this is a lock-less ring buffer advancement if this
            // is not a multi-threaded filter.
            //
            reader->readPtr += advanceLen;
            if(reader->readPtr >= reader->buffer->mem +
                    reader->buffer->mapLength)
                // Wrap back the circular buffer.
                reader->readPtr -= reader->buffer->mapLength;
            // Remove length from the input length tally.
            reader->readLength -= advanceLen;

            if(keepInputing) {
                // We will be calling input() again with these new args in
                // the current thread.  This has to keep using this thread
                // until it advances all the input buffers to a level less
                // than reader->maxRead in all input ports.
                //
                // If there was no advancelen for any input port than the
                // args for that port can stay the way they are. 
                j->inputLens[i] -= advanceLen;
                j->advanceLens[i] = 0;
                j->inputBuffers[i] = reader->readPtr;
            }
        }
    }

    return keepInputing;
}



// This is called just after filter input() was called in the working
// thread.
//
// We may need to vary how this function traverses the stream filter
// graph.  It may queue-up jobs for any adjacent filters, depending on how
// working threads are currently distributed across these adjacent
// filters.  Outputs from and Inputs into this filter, f, trigger queuing
// jobs into the adjacent (input and output) filters.
//
// This function needs to be as fast as possible.
//
// This function can spawn new worker threads.
//
// We must have the filter->mutex lock to call this.  This adds an inner
// stream lock to that, so it'll be double locked in here.
static inline
void PushJobsToStreamQueue(struct QsStream *s, struct QsFilter *f) {

    DASSERT(s);
    DASSERT(f);
    DASSERT(f->stream == s);

    // LOCK -- Now double  filter and then stream lock
    CHECK(pthread_mutex_lock(&s->mutex));

    // newJobs will count the number of new jobs that will get put into
    // the stream job queue.
    uint32_t newJobs = 0;

    // It's just a little bit faster to go through the filter outputs
    // backwards in port numbers because we do not have to setup a
    // numOutputs variable or dereference f in the top of the loop like
    // in: for(uint32_t i=0; i<f->numOutputs; ++i)
    //
    for(uint32_t i=f->numOutputs-1; i!=-1; --i) {

        struct QsOutput *output = f->outputs + i;

        DSPEW("%p", output);

    }

    if(f->isSource) {

        // TODO: this code.
    }

    // We cannot know how many threads actually wake up for each
    // pthread_cond_signal() call, we only know that at least one is
    // waken, so we need another idle thread counter to get an estimate.
    //
    uint32_t numIdleThreads = s->numIdleThreads;

    while(newJobs) {

        if(numIdleThreads) {
            // case 1. Steady state case would already have the max
            // threads spawned if it is possible, so we do the idle thread
            // case first.  Is better employ old workers than create new
            // workers.
            //
            // There is a thread calling pthread_cond_wait(s->cond, s->mutex),
            // the s->mutex lock before that guarantees it.
            //
            CHECK(pthread_cond_signal(&s->cond));

            // At least one (and maybe more) worker threads will wake up
            // sometime after we release the s->mutex.  The threads that wake
            // up will handle the numIdleThreads counting.  We are done.
            --newJobs;

        } else if(s->numThreads < s->maxThreads) {
            // case 2. We do not have max worker threads yet.
            //
            // Stream does not have its' quota of worker threads.
            //
            pthread_t thread;
            CHECK(pthread_create(&thread, 0/*attr*/,
                    (void *(*) (void *)) RunningWorkerThread, s));

            ++s->numThreads;
            --newJobs;
        } else
            // We have our quota of threads all working now.  The jobs are
            // in the stream queue, so they'll get to it in time as they
            // finish their current jobs.
            break;
    }

    CHECK(pthread_mutex_unlock(&s->mutex));
    // UNLOCK -- Inner stream unlocked, but we may still have a filter lock.
}



// Returns true to signal call me again, and false otherwise.
static inline
bool RunInput(struct QsStream *s, struct QsFilter *f, struct QsJob *j) {

    int inputRet;

    inputRet = f->input(j->inputBuffers, j->inputLens,
            j->isFlushing, f->numInputs, f->numOutputs);


    // FILTER LOCK  -- does nothing if not a multi-threaded filter
    CheckLockFilter(f);

    // Flag to mark that input() will be called again by this thread
    // without calling any other input() in between.  Even if that's not
    // the case, the input() may be called later by a thread (this or some
    // other) getting the job from the stream job queue.  We just need a
    // higher priority case where we call input() repeatedly to get the
    // input buffers depleted enough to prevent blocking the feeding
    // filters.
    bool willInputMore = false;

    if(inputRet == 0)
        willInputMore = ProcessInput(f, j, f->numInputs);
    else {
        //
        // This filter is done having input() called for this flow cycle
        // (start(), input(), input(), input(), ..., stop()) so we do not
        // need to mess with input any more.  The buffers/readers will be
        // reset before the next flow/run cycle, if there is one.  But we
        // still need to deal with output.
        //
        if(inputRet < 0)
            WARN("filter \"%s\" input() returned error code %d",
                    f->name, inputRet);
        // Mark this filter as being done having it's input() called.
        f->mark = true;
    }

    PushJobsToStreamQueue(s, f);


    // FILTER UNLOCK  -- does nothing if not a multi-threaded filter
    CheckUnlockFilter(f);

    return willInputMore;
}



// This is the function called by worker threads.
//
void *RunningWorkerThread(struct QsStream *s) {

    DASSERT(s);
    DASSERT(s->maxThreads);

    // The life of a worker thread.
    //
    bool living = true;

    // STREAM LOCK
    CHECK(pthread_mutex_lock(&s->mutex));

    // The thing that created this thread must have counted the number of
    // threads in the stream, otherwise if we counted it here and there
    // are threads that are slow to start, than the master thread could
    // see less threads than there really are.  So s->numThreads can't
    // be zero now.
    DASSERT(s->numThreads);
    // The number of threads must be less than or equal to stream
    // maxThreads.  We have the needed stream mutex lock to check this.
    DASSERT(s->numThreads <= s->maxThreads);


    DSPEW("The %" PRIu32 " (out of %" PRIu32
            " worker threads are running.",
            s->numThreads, s->maxThreads);


    // We work until we die.
    //
    while(living) {

        // Get the next job (j) from the stream job queue.
        //
        struct QsJob *j = StreamQToFilterWorker(s);


        if(j == 0) {
            // We are unemployed.  We have no job.  Just like I'll be,
            // after I finish writing this fucking code.

            // We count ourselves in the ranks of the unemployed.
            ++s->numIdleThreads;

            DSPEW("Now %" PRIu32 " out of %" PRIu32
                    "thread(s) waiting for work",
                    s->numIdleThreads, s->numThreads);

            // STREAM UNLOCK
            // wait
            CHECK(pthread_cond_wait(&s->cond, &s->mutex));
            // STREAM LOCK

            // Remove ourselves from the numIdleThreads.
            --s->numIdleThreads;

            DSPEW("Now %" PRIu32 " out of %" PRIu32
                    "thread(s) waiting for work",
                    s->numIdleThreads, s->numThreads);

            // Because there can be more than one thread woken by a
            // signal, we may still not have a job available for this
            // worker.  Such is life for the masses.
            continue;
        }

        struct QsFilter *f = j->filter;
        DASSERT(f);

        // This worker has a new job.
        //
        // This thread can now read and write to this job, j, without a
        // mutex.  No other thread will access this job while it is
        // here.

        // Put it in this thread specific data so we can find it in the
        // filter input() when this thread runs things like
        // qsAdvanceInput(), qsOutput(), and other quickstream/filter.h
        // functions.
        //
        CHECK(pthread_setspecific(_qsKey, j));


        // STREAM UNLOCK
        CHECK(pthread_mutex_unlock(&s->mutex));


        // call input() as many times as needed
        while(RunInput(s, f, j));


        // STREAM LOCK
        CHECK(pthread_mutex_lock(&s->mutex));

        // Move this job structure to the filter unused stack.
        FilterWorkingToFilterUnused(j);
    }


    // STREAM UNLOCK
    CHECK(pthread_mutex_unlock(&s->mutex));


    return 0; // We're dead now.  It was a good life.
}
