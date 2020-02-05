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
// In addition to the 3 job transfers shown in the above image, we
// sometimes at the end of a quickstream flow cycle will need to move jobs
// from the stream job queue back to the filter's unused stack, because
// a filters stops receiving data.  So we have for job (struct QsJob)
// transfer functions in this file.


// Holding the stream mutex lock is required for all 4 of these job list
// transfer functions.


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
// This is an unusual job list transfer function that only happens when a
// filter module finishes running unexpectedly and so we need to remove
// queued up jobs from the list that stream job queue that feeds worker
// threads.  This case is not shown on the "job flow graph".  Just think
// of it as a filter going into a stopped state, and some of the input
// data at the end in the remaining queued jobs is discarded.
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



// 1. Remove job from filter unused job stack.
//
// 2. transfer that job to stream job queue.
//
// 3. clean the job args.
//
// This is first called by the master thread with all the source filters.
// It is also called by worker threads to queue jobs in the order that
// the working threads traverse the stream filter graph.
//
// We must have a stream->mutex lock to call this.
static inline
void FilterUnusedToStreamQ(struct QsStream *s, struct QsFilter *f) {

    DASSERT(s);
    DASSERT(f);
    DASSERT(f->stream == s);

    if(f->mark) {
        // This filter is marked as done with this flow cycle, so we just
        // ignore this request.
        WARN("filter \"%s\" ignoring stream job queue request", f->name);
        return;
    }


    /////////////////////////////////////////////////////////////////////
    // 1. Remove job from filter unused job stack.

    struct QsJob *j = f->unused;
    DASSERT(j);
    DASSERT(j->prev == 0);

    // We do not queue up jobs for filters that have their fill of threads
    // already working.  Adding more jobs to the stream queue is not done
    // unless the filter that will receive the worker and job has the job
    // to give.
    DASSERT(f->numWorkingThreads < s->maxThreads);
    DASSERT(f->numWorkingThreads < f->maxThreads);
    DASSERT(f->numWorkingThreads < GetNumAllocJobsForFilter(s, f));

    f->unused = j->next;
    j->next = 0;


    /////////////////////////////////////////////////////////////////////
    // 2. Transfer that job to stream job queue.

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

 
    /////////////////////////////////////////////////////////////////////
    // 3. Now clean/reset the job args.

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

    if(f->numOutputs) {
        DASSERT(j->outputLens);
        memset(j->outputLens, 0, f->numOutputs*sizeof(*j->outputLens));
    }
#ifdef DEBUG
    else {
        DASSERT(j->outputLens == 0);
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
    // of the line (at the DMV (department of motor vehicles)).

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
        DASSERT(f->numWorkingThreads ==
                GetNumAllocJobsForFilter(f->stream, f) - 1);
#endif

    f->unused = j;

    --f->numWorkingThreads;

    // The job args will be cleaned up later in FilterUnusedToStreamQ().
}
