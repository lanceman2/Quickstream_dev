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




// This function must be thread-safe and restraint.
//
// Part of the user filter API.
void *qsGetOutputBuffer(uint32_t outputPortNum,
        size_t maxLen, size_t minLen) {


    // This is the other end of a goto retry; statement that is below in
    // this function.  It appeared that using more functions and a while
    // loop was much more convoluted code than the following with a goto.
retry: ;

    // Get the filter.
    struct QsJob *job = pthread_getspecific(_qsKey);
    ASSERT(job, "thread_getspecific(_qsKey) failed");
    DASSERT(job->magic == _QS_IS_JOB);
    struct QsFilter *f = job->filter;
    DASSERT(f);
    struct QsStream *s = f->stream;
    DASSERT(s);
    ASSERT(maxLen);
    ASSERT(maxLen >= minLen);
    DASSERT(f->input);
    DASSERT(f->numOutputs);
    DASSERT(f->outputs);
    ASSERT(f->numOutputs >= outputPortNum);

    //pthread_cond_t *cond = f->cond;
    struct QsOutput *output = f->outputs + outputPortNum;
    DASSERT(output->readers);
    DASSERT(output->numReaders);
    ASSERT(maxLen <= output->maxWrite);


    // STREAM LOCK
    CHECK(pthread_mutex_lock(&s->mutex));
    // We got a stream mutex lock so that we can look at all the reader
    // filter's stage jobs.

    // multi-threaded filters pay a toll.
    //
    // FILTER LOCK  -- does nothing if not a multi-threaded filter
    CheckLockFilter(f);
    // We get a filter mutex lock so we can access the changing parts of
    // this filter's, f, output structure.

    // TODO: add "pass-through" buffer support.

    // Look at all readers of this output.  None may have read lengths too
    // large, otherwise we are blocked from writing to the buffer.
    for(uint32_t i = output->numReaders - 1; i != -1; --i) {

        struct QsReader *reader = output->readers + i;
        struct QsFilter *readF = reader->filter;
        uint32_t inputPort = reader->inputPortNum;

        if(readF->stage->inputLens[inputPort] >= output->maxLength) {
            // This read filter needs to read more before we can return a
            // buffer pointer of the feeding filter, f.  Otherwise the
            // buffer read pointer could get overrun by the write pointer.
            // This is the BLOCKING case.
            ASSERT(0, "Write this code");

            

            // FILTER UNLOCK  -- does nothing if not a multi-threaded filter
            CheckUnlockFilter(f);


            // STREAM UNLOCK
            CHECK(pthread_mutex_unlock(&s->mutex));


            

            goto retry;
        }

    }
    ASSERT(0, "Write this code");

    // STREAM UNLOCK
    CHECK(pthread_mutex_unlock(&s->mutex));


    // FILTER UNLOCK  -- does nothing if not a multi-threaded filter
    CheckUnlockFilter(f);



    return output->writePtr;
}




// Returns true to signal call me again, and false otherwise.
static inline
bool RunInput(struct QsStream *s, struct QsFilter *f, struct QsJob *j) {


    // At this point this filter/thread owns this job.
    //
    // First we find where the filter buffer pointers from the QsReader
    // readPtr and readLength, and the job inputBuffers[] and job
    // inputLens[].
    //
    // This filter, f, owns readPtr and readLength, and so only this
    // filter, f, can read and write to readPtr and readLength.  That's
    // how we get lock-less buffers (but not in the multi-threaded filter
    // case).

    // multi-threaded filters pay a toll.
    //
    // FILTER LOCK  -- does nothing if not a multi-threaded filter
    CheckLockFilter(f);

    for(uint32_t i=f->numInputs; i!=-1; --i) {

        // Reset the job input() args:

        j->advanceLens[i] = 0;

        // The feed filter could not do this, because it
        // was not the owner of readers[i]->readPtr like the current
        // thread/filter.
        j->inputBuffers[i] = f->readers[i]->readPtr;

        // Add the leftover unread length to the length that the feed
        // filter gave us.  That feed filter could not do this, because it
        // was not the owner of readers[i]->readLength like the current
        // thread/filter, f.
        j->inputLens[i] += f->readers[i]->readLength;
    }

    // Initialize the record of output lengths to output buffers in this
    // thread.  This could just be a counter in the QsOutput, but we need
    // one of these counters for each thread, so it's in the job.
    for(uint32_t i=f->numOutputs; i!=-1; --i)
        j->outputLens[i] = 0;


    // FILTER UNLOCK  -- does nothing if not a multi-threaded filter
    CheckUnlockFilter(f);


    int inputRet;

    inputRet = f->input(j->inputBuffers, j->inputLens,
            j->isFlushing, f->numInputs, f->numOutputs);

    if(inputRet) {
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

 
    // If willInputMore==true this function is called again immediately.
    //
    return false;
}



// We require a stream mutex lock before calling this.
//
// This function returns while holding the stream mutex lock.
//
// returns 0 if all threads would be sleeping.
static inline
struct QsJob *WaitForWork(struct QsStream *s) {

#ifdef SPEW_LEVEL_DEBUG
    // So we may spew when the number of working threads changes.
    bool weSlept = false;
#endif

    while(true) {
    
        // We are unemployed.  We have no job.  Just like I'll be, after I
        // finish writing this code.

        // Get the next job (j) from the stream job queue is there is
        // one.
        //
        struct QsJob *j = StreamQToFilterWorker(s);

#ifdef SPEW_LEVEL_DEBUG
        // We only spew if the number of working threads has changed and
        // if we slept than the number of working threads has changed.
        if(weSlept && j)
            DSPEW("Now %" PRIu32 " out of %" PRIu32
                "thread(s) waiting for work",
                s->numIdleThreads, s->numThreads);
        else if(j == 0)
            weSlept = true;
#endif

        if(j) return j;

        if(s->numIdleThreads == s->numThreads - 1)
            // All other threads are idle so we are done working/living.
            return 0;

        // We count ourselves in the ranks of the sleeping unemployed.
        ++s->numIdleThreads;

        // TODO: all the spewing in this function may need to be removed.
        DSPEW("Now %" PRIu32 " out of %" PRIu32
                "thread(s) waiting for work",
                s->numIdleThreads, s->numThreads);

        // STREAM UNLOCK  -- at wait
        // wait
        CHECK(pthread_cond_wait(&s->cond, &s->mutex));
        // STREAM LOCK  -- when woken.

        // Remove ourselves from the numIdleThreads.
        --s->numIdleThreads;

        if(s->numIdleThreads == s->numThreads - 1)
            // All other threads are idle so we are done
            // working/living.
            return 0;
    }
}



// This is the first function called by worker threads.
//
void *RunningWorkerThread(struct QsStream *s) {

    DASSERT(s);
    DASSERT(s->maxThreads);

    // The life of a worker thread.

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

    struct QsJob *j;

    // We work until we die.
    //
    while((j = WaitForWork(s))) {
 
        // STREAM UNLOCK
        CHECK(pthread_mutex_unlock(&s->mutex));

        struct QsFilter *f = j->filter;
        DASSERT(f);

        // This worker has a new job.
        //
        // This thread can now read and write to this job, j, without a
        // mutex.  No other thread will access this job while it is
        // here.
        //
        // Put it in this thread specific data so we can find it in the
        // filter input() when this thread calls functions like
        // qsAdvanceInput(), qsOutput(), and other quickstream/filter.h
        // functions.
        //
        CHECK(pthread_setspecific(_qsKey, j));

        // call input()
        RunInput(s, f, j);

        // STREAM LOCK
        CHECK(pthread_mutex_lock(&s->mutex));

        // Move this job structure to the filter unused stack.
        FilterWorkingToFilterUnused(j);
    }

    // This thread is now no longer counted among the working/living.
    --s->numThreads;

    // Now this thread does not count in s->numThreads or
    // s->numIdleThreads
    if(s->numThreads && s->numIdleThreads == s->numThreads) {

        // Wake up the all these lazy workers so they can return.
        CHECK(pthread_cond_broadcast(&s->cond));
        // TODO: pthread_cond_broadcast() does nothing if it was called by
        // another thread on the way out.  Maybe add a flag so we do not
        // call it more than once in this case.
    }

    if(s->numThreads == 0 && s->masterWaiting)
        // this last thread out signals the master.
        CHECK(pthread_cond_broadcast(&s->masterCond));


    // STREAM UNLOCK
    CHECK(pthread_mutex_unlock(&s->mutex));


    return 0; // We're dead now.  It was a good life for a worker/slave.
}
