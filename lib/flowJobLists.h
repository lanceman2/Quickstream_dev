// This file contains inline functions that move struct QsJobs (jobs) from
// one list to another.  It shows how the jobs flow through different
// lists, be they queues, stacks, or single element pointers.
//
// These lists of jobs and the "flow of jobs" between them allow us to
// have the effect of threads flowing through the quickstream filter flow
// graph.  This is a what makes quickstream interesting, fast, and a
// self-optimising stream framework.  The other stream frameworks have
// threads fixed to their filters, and so the threads do not flow.  There
// are so many reasons why that is not optimal.  One can show that that
// will not be optimal unless the filter loads are balanced.  quickstream
// automatically balances the thread time to filter "load" ratio.
// Puts simply, worker threads are free to work where work is needed; they
// are not bound to one filter.


//  See the "job flow graph" by running (bash command-line):
//
//
//        display ../share/doc/quickstream/jobFlow.dot
//
//
//
// Notice in the "job flow graph" there are 4 lines connecting 4 types of
// job lists.  That gives us 4 possible transfer C functions, but we need
// only 3, because the 2 transfers 'filter stage' -> 'stream queue', and
// 'filter unused' -> 'filter stage' always happen at the same time.  The
// filter unused and the filter stage really act as one list, with the
// stage job being a special element in that list.
//
//
// So ya, these 3 job list transfer functions are in this file:



// Holding the stream mutex lock is required for all 3 job list transfer
// functions.



// 1. Transfer job from the filter->stage to the stream job queue, then
//
// 2. transfer a job from filter unused to filter stage (to replace the
//    one you just moved), and then
//
// 3. clean the filter stage job args.
//
//
// The filter stage is a queue of one.
//
// This is first called by the master thread with all the source filters.
// It is also called by worker threads after the jobs finish.
//
// We must have a stream->mutex lock to call this.
static inline
void FilterStageToStreamQAndSoOn(struct QsStream *s, struct QsJob *j) {

    DASSERT(s);
    DASSERT(j->next == 0);
    struct QsFilter *f = j->filter;
    DASSERT(f);
    DASSERT(f->stage);
    DASSERT(f->stage == j);
    DASSERT(f->unused);

    // We do not queue up jobs for filters that have there fill of threads
    // already working.  Adding more jobs to the stream queue is not done
    // unless the filter that will receive the worker and job has the job
    // to give.
    DASSERT(f->numWorkingThreads < s->maxThreads);
    DASSERT(f->numWorkingThreads < f->maxThreads);


    /////////////////////////////////////////////////////////////////////
    // 1. Transfer job from the filter->stage to the stream job queue.

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
    // 2. Now transfer a job from filter unused to filter stage.

    j = f->unused;
    f->stage = j;
    f->unused = j->next;


    /////////////////////////////////////////////////////////////////////
    // 3. Now clean/reset the filter stage job args.

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
// 1. remove the jobFirst job from the stream queue, and then
//
// 2. put the job in the filters working queue and return
//    that job.
//
// Also increments the filter's numWorkingThreads.
//
// We must have a stream->mutex lock to call this.
static inline
struct QsJob *StreamQToFilterWorker(struct QsStream *s) { 

    DASSERT(s);

    if(s->jobFirst == 0) return 0; // We have no job for them.

    /////////////////////////////////////////////////////////////////////
    // 1. remove the jobFirst job from the queue, and then

    struct QsJob *j = s->jobFirst;
    DASSERT(j->prev == 0);
    DASSERT(s->jobLast);
    DASSERT(s->jobLast->next == 0);
    s->jobFirst = j->next;
    if(s->jobFirst == 0) {
        // j->next == 0
        DASSERT(s->jobLast == j);
        s->jobLast = 0;
        // There are now no jobs in the stream job queue.
    } else {
        // There are still some jobs in the stream job queue.
        DASSERT(j->next);
        j->next = 0;
    }

    struct QsFilter *f = j->filter;
    DASSERT(f);

    /////////////////////////////////////////////////////////////////////
    // 2. Add this job to the filter working queue:

    // One more thread working for this filter.
    ++f->numWorkingThreads;

    // There should be at least 1 job more than those in use.
    // Better to do this check before changing pointers.
    DASSERT(GetNumAllocJobsForFilter(s, f) > f->numWorkingThreads);

    // Next points back to the next to be served, as in you will be served
    // after me who is in front of you, and prev points toward the front
    // of the line.

    if(f->workingLast) {
        // We had jobs in the queue
        DASSERT(f->numWorkingThreads > 1);
        // None after the last.
        DASSERT(f->workingLast->next == 0);
        DASSERT(f->workingFirst);
        f->workingLast->next = j;
        j->prev = f->workingLast;
    } else {
        // We had no jobs in the queue.
        DASSERT(f->workingFirst == 0);
        f->workingFirst = j;
        DASSERT(f->numWorkingThreads == 1);
    }

    f->workingLast = j;

    return j;
}


// This is called by the worker threads.
//
// Transfer job from the filter working queue to the filter unused
// stack.
//
//  1. Remove the job from the filter working queue, and then
//
//  2. Put the job into the filter unused stack.
//
// Also decrements the filter's numWorkingThreads.
//
// We must have a stream->mutex lock to call this.
static inline
void FilterWorkingToFilterUnused(struct QsJob *j) {

    DASSERT(j);
    struct QsFilter *f = j->filter;
    DASSERT(f);

    /////////////////////////////////////////////////////////////////////
    //  1. Remove the job from the filter working queue.

    // There must be a job in the working queue.
    DASSERT(f->workingLast);
    DASSERT(f->workingFirst);

    // The most probable job that has just finished would be at the front
    // of the queue, or first.
    if(j->prev == 0) {
        // The job is first in the filter working queue
        DASSERT(j == f->workingFirst);
        f->workingFirst = j->next;

        if(j->next) {
            DASSERT(f->numWorkingThreads > 1);
            DASSERT(j->next->prev == j);
            j->next->prev = 0;
            j->next = 0;
        } else {
            // j->next == 0
            DASSERT(f->numWorkingThreads == 1);
            DASSERT(f->workingLast == j);
            f->workingLast = 0;
            // The filter working queue will be empty
        }

    } else {
        // j->prev != 0
        //
        // The job must not be first in the queue.
        DASSERT(j != f->workingFirst);
        DASSERT(f->numWorkingThreads > 1);

        j->prev->next = j->next;

        if(j->next) {
            DASSERT(f->numWorkingThreads > 1);
            j->next->prev = j->prev;
            j->next = 0;
        } else {
            // j->next == 0
            DASSERT(f->workingLast == j);
            f->workingLast = j->prev;
        }
        j->prev = 0;
    }

    --f->numWorkingThreads;


    /////////////////////////////////////////////////////////////////////
    //  2. Put the job into the filter unused stack.

    if(f->unused) {
        DASSERT(f->numWorkingThreads <
                GetNumAllocJobsForFilter(f->stream, f));
        j->next = f->unused;
    } else {
        f->unused = j;
    }

    // The job args will be cleaned up later in FilterUnusedToFilterQ().
}


static inline
void CheckLockOutput(struct QsFilter *f) {
    if(f->mutex)
        CHECK(pthread_mutex_lock(f->mutex));
}


static inline
void CheckUnlockOutput(struct QsFilter *f) {
    if(f->mutex)
        CHECK(pthread_mutex_unlock(f->mutex));
}
