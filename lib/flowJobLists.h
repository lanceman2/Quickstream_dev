// This file contains inline functions that move struct QsJobs (jobs) from
// one list to another.  It shows how the jobs flow through different
// lists, be they queues, stacks, or single element pointers.
//
// These lists of jobs and the "flow of jobs" between them allow us to
// have the effect of threads flowing through the quickstream filter flow
// graph.  That is what makes quickstream interesting, fast, and a
// self-optimising stream framework.  The other stream frameworks have
// threads fixed to their filters, and so the threads do not flow.  There
// are so many reasons why that is not optimal.  One can show that that
// will not be optimal unless the filter loads are balanced.
// quickstream automatically balances the thread time to filter "load"
// ratio.  Puts simply, worker threads are free to work where work is
// needed; they are not bound to one filter.


//  See the "job flow graph" by running (bash command-line):
//
//
//        display ../share/doc/quickstream/jobFlow.dot
//
//  or see it on the web at
//
//    https://raw.githubusercontent.com/lanceman2/quickstream.doc/master/jobFlow.png
//
//
//
// Notice in the "job flow graph" there are 4 lines connecting 4 types of
// job lists.  That gives us 4 possible transfer C functions, but we need
// only 3, because the 2 transfers 'filter stage' -> 'stream queue' and
// 'filter unused' -> 'filter stage' always happen at the same time.  The
// filter unused and the filter stage really act as one list, with the
// stage job being a special element in that list.
//
//
// So ya, there are 3 job list transfer functions are in this file.



// Holding the stream mutex lock is required for all 3 job list transfer
// functions.


// The order that the working threads traverse the stream filter graph is
// something that could be variable and very important.

// By writing this code in a somewhat agile manner, it would appear that
// we have found that these job lists are an emergent propriety of the
// code.  We only required that the threads be able to move from one
// filter to the next, that we use no allocations as the working threads
// move from filter to filter, and the code and data structures be
// minimal.



// This removes a job from the stream job queue and puts it in the filter
// unused job stack.  It's used to clear jobs out of order, because the
// filter has stopped having it's input() called.  The job must exist in
// the stream job queue.
//
//  1. Remove job from stream job queue.
//
//  2. Put the job into the filter unused job stack.
//
// We must have a stream->mutex lock to call this.
static inline
void StreamQToFilterUnused(struct QsStream *s, struct QsFilter *f,
        struct QsJob *j) {

    DASSERT(f);
    DASSERT(s);
    DASSERT(f->stream == s);
    DASSERT(j);
    DASSERT(j->filter == f);

    /////////////////////////////////////////////////////////////////////
    // 1. Remove job from stream job queue.

    if(j->next) {
        DASSERT(j != s->jobLast);
        j->next->prev = j->prev;
    } else {
        DASSERT(j == s->jobLast);
        s->jobLast = j->prev;
    }
    if(j->prev) {
        DASSERT(j != s->jobFirst);
        j->prev->next = j->next;
        j->prev = 0;
    } else {
        DASSERT(j == s->jobFirst);
        s->jobFirst = j->next;
    }


    /////////////////////////////////////////////////////////////////////
    //  2. Put the job into the filter unused job stack.

    j->next = f->unused;
    f->unused = j;

}



// 1. Transfer job from the filter->stage to the stream job queue, then
//
// 2. transfer a job from filter unused to filter stage (to replace the
//    one you just moved), and then
//
// 3. clean the filter stage job args.
//
//
// Steps 2. and 3. are the "AndSoOn" part of the function name.
//
// The filter stage is a queue of one.
//
// This is first called by the master thread with all the source filters.
// It is also called by worker threads to queue jobs in the order that
// the working threads traverse the stream filter graph.
//
// We must have a stream->mutex lock to call this.
static inline
void FilterStageToStreamQAndSoOn(struct QsStream *s, struct QsFilter *f) {

    DASSERT(s);
    DASSERT(f);
    DASSERT(f->stream == s);

    if(f->mark) {
        // This filter is marked as done with this flow cycle, so we just
        // ignore this request.
        WARN("filter \"%s\" ignoring stream job queue request", f->name);
        return;
    }


    struct QsJob *j = f->stage;
    DASSERT(j);
    DASSERT(j->next == 0);
    DASSERT(j->prev == 0);
    DASSERT(f->unused);

    // We do not queue up jobs for filters that have their fill of threads
    // already working.  Adding more jobs to the stream queue is not done
    // unless the filter that will receive the worker and job has the job
    // to give.
    DASSERT(f->numWorkingThreads < s->maxThreads);
    DASSERT(f->numWorkingThreads < f->maxThreads);
    DASSERT(f->numWorkingThreads < GetNumAllocJobsForFilter(s, f));


    /////////////////////////////////////////////////////////////////////
    // 1. Transfer job from the filter->stage to the stream job queue.

    if(s->jobLast) {
        // There are jobs in the stream queue.
        DASSERT(s->jobLast->next == 0);
        DASSERT(s->jobFirst);
        s->jobLast->next = j;
    } else {
        // There are no jobs in the stream queue.
        DASSERT(s->jobFirst == 0);
        s->jobFirst = j;
    }

    s->jobLast = j;

    // We are done with this j.  It's queued.  We can reuse variable j.

    /////////////////////////////////////////////////////////////////////
    // 2. Now transfer a job from filter unused to filter stage.

    j = f->unused;
    f->stage = j;
    f->unused = j->next;
    if(j->next)
        j->next = 0;

    DASSERT(j->prev == 0);


    /////////////////////////////////////////////////////////////////////
    // 3. Now clean/reset the filter new stage job args.

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


// This is called only by the worker threads.
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
// This function feeds jobs to the workers threads in the order that the
// worker threads traverse the stream filter graph.
//
// We must have a stream->mutex lock to call this.
static inline
struct QsJob *StreamQToFilterWorker(struct QsStream *s) { 

    DASSERT(s);

    if(s->jobFirst == 0) return 0; // We have no job for them.

    /////////////////////////////////////////////////////////////////////
    // 1. remove the jobFirst job from the stream queue

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
        j->next = 0;
    }

    struct QsFilter *f = j->filter;
    DASSERT(f);
    DASSERT(f->stream == s);

    /////////////////////////////////////////////////////////////////////
    // 2. Add this job to the filter working queue:

    // One more thread working for this filter.
    ++f->numWorkingThreads;

    // There should be at least 1 job more than those in use.
    // Better to do this check before changing pointers.
    DASSERT(GetNumAllocJobsForFilter(s, f) > f->numWorkingThreads);

    // Next points back to the next to be served, as in you will be served
    // after me who is in front of you, and prev points toward the front
    // of the line (at the DMV).

    if(f->workingLast) {
        // We had jobs in the working queue
        DASSERT(f->numWorkingThreads > 1);
        // None after the last.
        DASSERT(f->workingLast->next == 0);
        DASSERT(f->workingFirst);
        f->workingLast->next = j;
        j->prev = f->workingLast;
    } else {
        // We had no jobs in the working queue.
        DASSERT(f->workingFirst == 0);
        f->workingFirst = j;
        DASSERT(f->numWorkingThreads == 1);
    }

    f->workingLast = j;


#ifdef DEBUG
    // "I'm a dumb-ass" check.
    if(f->unused == 0) {
        // With no unused, see if working queue length is at it's
        // maximum.
        if(s->maxThreads && f->maxThreads > s->maxThreads)
            DASSERT(s->maxThreads == f->numWorkingThreads);
        else
            DASSERT(f->maxThreads == f->numWorkingThreads);
    }
#endif

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
    // of the queue, or first: giving j->prev == 0
    //
    if(j->prev == 0) {
        // The job is first in the filter working queue
        DASSERT(j == f->workingFirst);
        f->workingFirst = j->next;
    } else {
        // j->prev != 0
        //
        // The job must not be first in the queue.
        DASSERT(j != f->workingFirst);
        DASSERT(f->numWorkingThreads > 1);
        j->prev->next = j->next;
        j->prev = 0;
    }

    if(j->next) {
        // The job must not be last in the queue.
        DASSERT(j != f->workingLast);
        DASSERT(f->numWorkingThreads > 1);
        j->next->prev = j->prev;
        j->next = 0;
    } else {
        // j->next == 0
        //
        // The job must be last in the queue.
        DASSERT(j == f->workingLast);
        DASSERT(f->numWorkingThreads == 1);
        f->workingLast = j->prev;
        // The filter working queue will be empty
    }


    --f->numWorkingThreads;


    /////////////////////////////////////////////////////////////////////
    //  2. Put the job into the filter unused stack.

    if(f->unused) {
        DASSERT(f->numWorkingThreads <
                GetNumAllocJobsForFilter(f->stream, f) - 1);
        j->next = f->unused;
    } 
#ifdef DEBUG
    else
        // one job in stage and the rest in working.
        DASSERT(f->numWorkingThreads == GetNumAllocJobsForFilter(f->stream, f) - 1);
#endif

    f->unused = j;


    // The job args will be cleaned up later in FilterStageToStreamQAndSoOn().
}


static inline
void CheckLockFilter(struct QsFilter *f) {
    if(f->mutex)
        CHECK(pthread_mutex_lock(f->mutex));
}


static inline
void CheckUnlockFilter(struct QsFilter *f) {
    if(f->mutex)
        CHECK(pthread_mutex_unlock(f->mutex));
}
