/* This is a private header file that is only used in the quickstream code
 * internal to libquickstream.so.  The libquickstream.so user, in general,
 * should not include this header file.  Hence this file is not installed
 * and is kept with the lib/libquickstream.so source code.
 */


// This file is the guts of quickstream.  By understanding each data
// member in these data structures you can get a clear understanding of
// the insides of quickstream, and code it.


// Then studying these structures one must keep in mind most data is
// constant at stream flow-time, and just some of the data is changing
// at stream flow-time.  When we have multiple threads accessing this
// data we can have only certain threads make changes to certain data,
// and avoid thread synchronization primitives, and other system calls.


// This CPP macro function CHECK() is just so we can call most pthread_*()
// (pthread_mutex_init() for example) and maybe other functions that
// return 0 on success and an int error number on failure, and asserts on
// failure printing errno (from ASSERT()) and the return value.  This is
// not so bad given this does not obscure what is being run.  Like for
// example:
//
//   CHECK(pthread_mutex_lock(&s->mutex));
//
// You can totally tell what that is doing.  One line of code instead of
// three.  Note (x) can only appear once in the macro expression,
// otherwise (x) could get executed more than once if it was listed more
// than once.
//
// CHECK() is not a debugging thing; it's inserting the (x) code every
// time.  And the same goes for ASSERT().
//
#define CHECK(x) \
    do { \
        int ret = (x); \
        ASSERT(ret == 0, #x "=%d FAILED", ret); \
    } while(0)



// For qsStreamLaunch(struct QsStream *s, uint32_t maxThreads) it is the
// maximum of maxThreads.  This value is somewhat arbitrary.
//
#define _QS_STREAM_MAXMAXTHREADS (213)

// bit Flags for the stream
//
// this is a stream configuration option bit flag
#define _QS_STREAM_ALLOWLOOPS        (01)


// This is the value of the stream flag when the stream is first created.
#define _QS_STREAM_DEFAULTFLAGS      (0)

// set if qsStreamLaunch() was called successfully
// unset in qsStreamStop()
//
// this is a stream state bit flag
#define _QS_STREAM_LAUNCHED          (02)


#define _QS_STREAM_START         (010)
#define _QS_STREAM_STOP          (020)

#define _QS_STREAM_MODE_MASK   (_QS_STREAM_START | _QS_STREAM_STOP)


// We cannot use the filter's stream when in the construct() and destroy()
// calls, so reuse the filter mark as a flag when we are in the
// construct() and destroy() calls.  So long as the values unique that
// should be okay.
//
// For in construct() filter->mark == _QS_IN_CONSTRUCT
// For in destroy()   filter->mark == _QS_IN_DESTROY
//
#define _QS_IN_CONSTRUCT        ((uint32_t) 21)
#define _QS_IN_DESTROY          ((uint32_t) 31)




// By limiting the number of channels possible, ports in or out we can use
// stack allocation via alloca().  We do not do something stupid like make
// arrays of this size.  We figured that a streaming API that allowed a
// million ports would not be that useful.
//
#define _QS_MAX_CHANNELS              ((uint32_t) (128))

// So we don't run away mapping to much memory for ring-buffers.  There
// just has to be some limit.
//
#define _QS_MAX_BUFFERLEN             ((size_t) (16*4*1024))




///////////////////////////////////////////////////////////////////////////
//
//                   THREAD SAFETY WARNING
//
//  Most of the variables in the data structures are not changing when the
//  stream is flowing.  Any variables that do change at flow time we must
//  handle with "thread-safe" care.
//
///////////////////////////////////////////////////////////////////////////


// In many cases, especially at stream flow-time, we use a data structure
// call an array for the fastest possible access to data in these data
// structures.  The cost of using an array is realloc()ing at
// non-flow-time, which is infrequent (transient case).  Using a tree or
// hash table is slower than accessing an array, so long as the array is
// not changing size, like it is not at flow-time.



// App (QsApp) is the top level quickstream object.  It's a container for
// filters and streams.  Perhaps there should only be one app in a
// program, but we do not impose that.  There is no compelling reason to
// limit the number apps that a program can have.  App is used to create
// filters by loading module plugins.  App is used to create streams.
//
struct QsApp {

    // There's no reason for fast access to these list at this time.
    // Maybe we'll use a red/black tree for faster access if the need
    // comes be.

    // List of filters.  Head of a singly linked list.
    struct QsFilter *filters;

    // We could have more than one stream.  We can't delete or edit one
    // while it is running.  You could do something weird like configure
    // one stream while another stream is running.
    //
    // List of streams.  Head of a singly linked list.
    struct QsStream *streams;
};


// For pthread_getspecific() and pthread_setspecific().  One hopes that
// pthread_getspecific() and pthread_setspecific() are very fast and do
// not cause a mode switch (system call) each time they are called.  The
// man page says "Performance and ease-of-use of pthread_getspecific() are
// critical".  If they do cause a mode switch we need to recode this.  One
// can't say without looking at the code or running tests; for example
// look at how slow system 5 semaphores are.
extern
pthread_key_t _qsKey;


// https://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Atomic-Builtins.html
// example: _sync_fetch_and_add()
// Or use:
// #include <stdatomic.h>
// example: https://en.cppreference.com/w/c/language/atomic



#ifdef DEBUG
extern
pthread_t _qsMainThread;

// We can use it like so:
// DASSERT(_qsMainThread == pthread_self(), "Not main thread");
// This may be needed for non-debug building in the future.

#endif



// Stream (QsStream) is the thing the manages a group of filters and their
// flow state.  Since streams can add and remove filters when it is not
// flowing the stream needs app to be a list of loaded filters for it.
//
struct QsStream {

    struct QsApp *app;

    // maxThreads=0 means do not start any.  maxThreads does not change at
    // flow/run time, so we need no mutex to access it.
    uint32_t maxThreads; // We will not create more pthreads than this.


    uint32_t flags; // bit flags that configure the stream
    // example the bit _QS_STREAM_ALLOWLOOPS may be set to allow loops
    // in the graph.  flags does not change at flow/run time, so we need
    // no mutex to access it at flow/run time.

    // We can define and set different flow() functions that run the flow
    // graph different ways.  This function gets set in qsStreamReady().
    //
    uint32_t (*flow)(struct QsStream *s);


    //////////////////// STREAM MUTEX GROUP ///////////////////////////////
    //
    // These are the only variables in QsStream that change at flow/run
    // time.
    //
    // This mutex protects the reading and writing of things that can
    // change in this stream struct when the stream is flowing.  There is
    // not much time needed to access the little changing data in the
    // stream, relatively speaking, or so we hope.  We expect a write/read
    // lock would not be as good as a simple mutex, given the low
    // probability of inter thread contention on this mutex.
    //
    // TODO: We can add the mutex contention test with
    // pthread_mutex_trylock() when we compile with DEBUG.
    //
    pthread_mutex_t mutex;
    // cond is paired with mutex.
    pthread_cond_t cond;  // idle threads just wait with this cond.
    // The main (master) thread waits on this conditional while the worker
    // threads run the stream flow.
    pthread_cond_t masterCond;
    bool masterWaiting;
    //
    // numThreads is the number of pthreads that exist from this stream,
    // be they idle or in the process of calling input().  We must have a
    // stream mutex lock to access numThreads.
    uint32_t numThreads;
    //
    // We do not need to keep list of idle threads.  We just have all idle
    // threads call pthread_cond_wait() with the above mutex and cond.
    //
    // numIdleThreads is the number of thread that at in the idle thread
    // list.  Threads in the idle thread list are blocking by calling
    // pthread_cond_wait(&stream->cond, &stream->mutex), so we do not need
    // a list of them, we just need to know if there are any and we just
    // signal to wake up one of them.
    //
    // numThreads - numIdleThreads = "number of threads in use".
    uint32_t numIdleThreads;
    //
    // jobQueue is the job that is being passed from the main thread to a
    // worker thread.
    struct QsJob *jobFirst; // next job in the streams job queue
    struct QsJob *jobLast; // last job in the streams job queue
    //
    //
    ///////////////////////////////////////////////////////////////////////


    // The array list of sources is created at start:
    uint32_t numSources;       // length of sources
    //
    // We also use sources==0 (or source != 0) as a flag that shows that
    // the stream flow-time resources have not been allocated yet (or
    // not), in place of introducing another flag.
    //
    // malloc()ed array of filter without input connections
    struct QsFilter **sources;

    // This list of filter connections is not used while the stream is
    // running (flowing).  It's queried a stream start, and the QsFilter
    // data structs are setup at startup.  The QsFilter data structures
    // are used when the stream is running.
    //
    // tallied filter connections:
    struct QsConnection {

        struct QsFilter *from; // from filter
        struct QsFilter *to;   // to filter

        uint32_t fromPortNum; // output port number for from filter
        uint32_t toPortNum;   // input port number for to filter

    } *connections; // malloc()ed array of connections


    uint32_t numConnections;// length of connections array

    struct QsStream *next; // next stream in app list of streams
};



// The filter (QsFilter) is loaded by app as a module DSO (dynamic shared
// object) plugin.  The filter after be loaded is added to a only one
// stream at a time.  If the filter is added to another stream, it will be
// removed from the previous stream; so ya, a filter can be only in one
// stream at a time.  When finished with a filter, the filter is unloaded
// by its' app.  quickstream users can write filters using
// include/qsfilter.h.  The quickstream software package also comes with a
// large selection of filters.
//
struct QsFilter {

    void *dlhandle; // from dlopen()

    struct QsApp *app;       // This does not change
    struct QsStream *stream; // This stream can be changed

    // name never changes after filter loading/creation pointer to
    // malloc()ed memory.
    char *name; // unique name for the Filter in a given app

    // Callback functions that may be loaded.  We do not get a copy of the
    // construct() and destroy() functions because they are only called
    // once, so we just dlsym() (if we have a dlhandle) them just before
    // we call them.
    int (* start)(uint32_t numInputs, uint32_t numOutputs);
    int (* stop)(uint32_t numInputs, uint32_t numOutputs);
    int (* input)(void *buffer[], const size_t len[],
            // If isFlushing[inPort]==true is this is the last bytes of
            // data in that input port.
            const bool isFlushing[],
            uint32_t numInputs, uint32_t numOutputs);


    struct QsFilter *next; // next loaded filter in app list


    ///////////////////////////////////////////////////////////////////////
    struct QsJob {

        ///////////////// STREAM MUTEX GROUP ////////////////////////////
        //
        // to put in filter unused stack or stream queue
        struct QsJob *next;
        //
        // The thread working on a given filter have an ID that is from a
        // sequence counter.  This threadNum (ID) is gotten from
        // filter->nextThreadNum++.  So the lowest numbered thread job for
        // this filter is the oldest, except when the counter wraps
        // through 0.  So really the oldest job is just the first in the
        // threadNum count sequence, and we must use filter->nextThreadNum
        // - filter->numThreads to get the oldest job threadNum.
        //
        // Hence, threadNum is respect to the filter, so that we can order
        // the threads with the buffers.
        uint32_t threadNum;
        //
        /////////////////////////////////////////////////////////////////

        // The number of inputs can change before start and after stop,
        // so can maxThread in the stream and so jobs and these input()
        // arguments are all reallocated at qsStreamLaunch().
        //
        // The inputBuffers[i], inputLens[i] i=0,1,2,..N-1 numInputs
        // pointer values can only be changed in qsOutput() by the feeding
        // filter's thread.  If the feeding filter can have more than one
        // thread than the thread that is changing them will lock the
        // output mutex.
        //
        // The arguments to pass to the input() call.  All are indexed by
        // input port number
        void **inputBuffers; // allocated after start and freed at stop
        size_t *inputLens;   // allocated after start and freed at stop
        bool *isFlushing;    // allocated after start and freed at stop
        //
        // advanceLens is the length in bytes that the filter advanced the
        // input buffers, indexed by input port number.
        size_t *advanceLens; // allocated after start and freed at stop
        //


        // This will be the pthread_getspecific() data for each flow
        // thread.  Each thread just calls the filter (QsFilter) input()
        // function.  When there is more than on thread calling a filter
        // input() we need to know things, QsJob, about that thread in
        // that input() call.

        // filter's who's input() is called for this job.
        // This filter is the filter structure that this job is in.
        struct QsFilter *filter;

    } *jobs; // The memory allocated for jobs in jobQueue, or unused.


    ///////////////// FILTER MUTEX GROUP ////////////////////////////////
    //
    // mutex for when more than one thread can run the filter input().
    //
    pthread_mutex_t *mutex;
    //
    // This filter owns these output structs, in that it is the only
    // filter that may change the ring buffer pointers.
    //
    uint32_t numOutputs; // number of connected filter we write to.
    struct QsOutput *outputs; // array of struct QsOutput
    //
    //
    uint32_t numInputs; // number of connected input filters feeding this.
    //
    // When filter->maxThreads > 1 we require a filter mutex lock to
    // access, r/w, a readPtr in readers[i]->readPtr.  The reader data
    // is in the output (QsOutput) of the feeding filter.  We just point
    // to the readers for this filter with readers.
    //
    // readers is an array of size filter->numInputs.  readers=0 if
    // numInput=0.
    //
    //
    //
    struct QsReader {
        //
        // readPtr points to a location in the mapped memory, "ring
        // buffer".
        //
        // After initialization, readPtr and readLength is only read and
        // written by the reading filter.  We use a reading filter mutex
        // for multi-thread filters input()s, but otherwise the buffer is
        // lock-less.
        uint8_t *readPtr;
        //
        // The amount of data that is available to be read from readPtr.
        size_t readLength;

        // The filter that is reading.
        struct QsFilter *filter;

        // This threshold will trigger a filter->input() call, independent
        // of the other inputs.
        //
        // The filter->input() may just return without advancing any input
        // or output, effectively ignoring the input() call like the
        // threshold trigger conditions where not reached.  In this way
        // we may have arbitrarily complex threshold trigger conditions.
        size_t threshold; // Length in bytes to cause input() to be called.

        // The reading filter promises to read some input data so long as
        // the buffer input length >= maxThreshold.  It only has to read 1
        // byte to fore fill this promise, but so long as the readable
        // amount of data on this port is >= maxThreshold is will keep
        // having it's input() called until the readable amount of data on
        // this port is < maxThreshold.
        //
        // This parameter guarantees that we can calculate a fixed ring
        // buffer size that will not be overrun.
        //
        // This parameter is used for "pass through" buffers in place of
        // maxWrite, since only one parameter is needed
        //
        size_t maxRead; // Length in bytes to keep input() being called.

        // The input port number that this filter being written to sees in
        // it's input(,,portNum,) call.
        uint32_t inputPortNum;
    }
    // array of pointers to readers array that is in feed filter
    // outputs.  Indexed by input port number 0, 1, 2, 3, ..., numInputs-1
    **readers;
    //
    /////////////////////////////////////////////////////////////////////


    ///////////////// STREAM MUTEX GROUP ////////////////////////////////
    //
    // All jobs in this filter are in one of these two job lists, or are
    // running with threads in the stream.
    //
    // When a working thread finishes a job it is this now the "finished
    // worker" thread that will put the job into the filters unused stack.
    // This guarantees that the worker thread can access the job in the
    // filter input() call without a mutex lock and without concern for
    // memory corruption.  The pointer to this "job" is a thread specific
    // attribute.
    //
    // Queue always points to one job.  The job may have no input buffer
    // data or it may have input buffer data.  There can only be
    // maxThreads threads with jobs; so we have maxThreads jobs and that
    // leaves at least one in the queue, and if there are less than
    // maxThreads with jobs, then there will be left over jobs in
    // "unused".
    //
    struct QsJob *queue;  // queue of one job that is waiting.
    struct QsJob *unused; // stack of 0 or more unused jobs.
    //
    // There can be maxThreads calling input() using maxThreads jobs for
    // this filter and one job that is "queued" that is not calling
    // input(); therefore the length of jobs is maxThreads + 1.  If not
    // thread-safe maxThreads is 1.  maxThreads will be fixed at flow/run
    // time.
    //
    // If stream->maxThread == 0 then jobs will not be used and buffers,
    // lens, and isFlushing will be allocated on the stack that is running
    // this flow.
    //
    // We exclude the case maxThreads=0 and use maxThreads=1.
    //
    // At flow time, array "jobs" has length maxThreads+1.
    //
    uint32_t maxThreads; // per this filter.
    //
    //
    // numThreads and nextThreadNum are accessed with stream mutex locked.
    //
    // numThreads is the number of threads currently calling
    // filter->input().  nextThreadNum is the thread number of the next
    // thread to call filter->input().  The leading thread number is then
    // nextThreadNum - numThreads.  Note this works when these numbers
    // wrap through zero.  When a new thread calls filter->input() it's
    // thread number will be nextThreadNum and then we add 1 to
    // nextThreadNum.
    //
    // We must have stream mutex locked to read or write numThreads or
    // nextThreadNum.
    uint32_t numThreads;
    uint32_t nextThreadNum;
    //
    /////////////////////////////////////////////////////////////////////
 

    // We define source as a filter with no input.  We will feed is zeros
    // when the stream is flowing.
    bool isSource; // startup flag marking filter as a source

    // TODO: It'd be nice not to have this extra data.  It's not needed
    // when the stream is flowing.
    //
    // mark is extra data in this struct so that we can save a marker
    // as we look through the filters in the graph, because if the graph
    // has cycles in it, it's not easy to look at each filter just once
    // without a marker to mark a filter as looked at.
    uint32_t mark;
};



struct QsOutput {  // points to reader filters

    // Outputs (QsOutputs) are only accessed by the filters (QSFilters)
    // that own them.


    // The "pass through" buffers are a double linked list with the "real"
    // buffer in the first one in the list.  The "real" output buffer has
    // prev==0.
    //
    // If this a "pass through" output prev points to the in another
    // filter output that this output uses to buffer its' data.
    //
    // So if prev is NOT 0 this is a "pass through" output buffer and prev
    // points toward the up-stream origin of the output thingy; if prev is
    // 0 this is not a "pass through" buffer.
    //
    struct QsOutput *prev;
    //
    // points to the next "pass through" output, if one is present; else
    // next is 0.
    struct QsOutput *next;


    // If prev is set this is a "pass through" and buffer points to the
    // "real" buffer that is the feed buffer, that feeds this pass through
    // buffer.
    //
    struct QsBuffer *buffer;

    // writePtr points to where to write next in mapped memory.  This
    // variable is not used if this is a "pass through" buffer.
    //
    // writePtr can only be read from and written to by the filter that
    // feeds this output.  If the filter that owns this output can run
    // input() in multiple threads a filter mutex lock is required to read
    // or write to this writePtr, but otherwise this is a lock-less
    // buffer.
    //
    uint8_t *writePtr;

    // The filter that owns this buffer promises to not write more than
    // maxWrite bytes to this buffer.
    //
    // Variable maxWrite is not used in a "pass through" output.  It will
    // just need read pointers, for read and write access to the same
    // place in memory (virtual address space memory).
    //
    size_t maxWrite;


    // readers is where the output data flows to.
    //
    // Elements in this array are accessible from the QsFilter (filter)
    // but filter->readers (not output->readers) are indexed by the
    // filters input port number.  This is readers of the current output
    // and the index number to this array is likely not related to the
    // reading filters input port number.
    //
    // Each element in this array may point to a different filter.
    //
    // This readers is a mapping from output to filter.  The QsFilter
    // structure has a readers array of pointers that is a mapping from
    // filter input port to reader (or output buffer).  Yes, it's
    // complicated but essential, so draw a pointer graph picture.
    //
    struct QsReader *readers; // array of filter readers.

    uint32_t numReaders; // length of readers array

    // input() just returns 0 if a threshold is not reached.
};


struct QsBuffer {

    ///////////////////////////////////////////////////////////////////////
    // The rest of this structure stays constant while the stream is
    // flowing.
    //
    uint8_t *mem; // Pointer to start of mmap()ed memory.

    //
    // These two parameters make it a circular buffer or ring buffer.  See
    // makeRingBuffer.c.  The mapLength is the most the buffer can hold to
    // be read by a filter.
    size_t mapLength, overhangLength; // in bytes.
};


// The filter that is having construct(), start(), stop(), or destroy()
// called.  There is only one thread when they are called.
//
// TODO: This makes the API not thread-safe.
extern
struct QsFilter *_qsCurrentFilter;


// These below functions are not API user interfaces:'



extern
void AllocateBuffer(struct QsFilter *f);


extern
void FreeBuffers(struct QsFilter *f);


extern
void MapRingBuffers(struct QsFilter *f);


extern
int stream_run_0p_0t(struct QsStream *s);


extern
void *makeRingBuffer(size_t *len, size_t *overhang);

extern
void freeRingBuffer(void *x, size_t len, size_t overhang);


extern
void CheckBufferThreadSync(struct QsStream *s, struct QsFilter *f);


extern
void SetPerThreadData(struct QsJob *job);


extern
void ReallocateFilterArgs(struct QsFilter *f, uint32_t num);



#if 0 // Not needed yet.
// Set filter->mark = val for every filter in the app.
static inline
void AppSetFilterMarks(struct QsApp *app, bool val) {
    for(struct QsFilter *f=app->filters; f; f=f->next)
        f->mark = val;
}
#endif

// Set filter->mark = val for every filter in this stream.
static inline
void StreamSetFilterMarks(struct QsStream *s, bool val) {

    if(s->numSources) {
        // Case stream has data structures build for running the flow.
        for(uint32_t i=0; i<s->numSources; ++i)
            s->sources[i]->mark = val;
        for(uint32_t i=0; i<s->numConnections; ++i)
            s->connections[i].to->mark = val;
        return;
    }

    // Case where the stream is not setup to flow yet, so we have not
    // found the source filters (sources) yet.  The filters are just
    // listed in the stream connection arrays, to[], and from[].
    for(uint32_t i=0; i<s->numConnections; ++i) {
        s->connections[i].to->mark = val;
        s->connections[i].from->mark = val;
    }
}
