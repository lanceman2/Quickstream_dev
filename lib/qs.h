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
// and maybe other functions that return 0 on success and an int error
// number on failure, and asserts on failure printing errno (from
// ASSERT()) and the return value.  This is not so bad given this does not
// obscure what is being run.  Like for example:
//
//   CHECK(pthread_mutex_destroy(&s->mutex));
//
// You can totally tell what that is doing.  One line of code instead of
// three.  Note (x) can only appear once in the macro expression,
// otherwise (x) could get executed more than once, if it is listed more
// than once.
//
#define CHECK(x) \
    do { \
        int ret = (x); \
        ASSERT(ret == 0, #x "=%d FAILED", ret); \
    } while(0)



#define _QS_STREAM_MAXMAXTHTREADS (80)

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

// By limiting the number of channels possible, ports in or out we can use
// stack allocation via alloca().
#define _QS_MAX_CHANNELS              ((uint32_t) (128))

// So we don't run away mapping to much memory for ring-buffers.  There
// just has to be some limit.
#define _QS_MAX_BUFFERLEN             ((size_t) (16*4*1024))


#define _QS_DEFAULT_MAXWRITELEN       ((size_t) 2*1024)


#define _QS_DEFAULT_THRESHOLD         ((size_t) 1)



///////////////////////////////////////////////////////////////////////////
//                 THREAD SAFETY WARNING
//
// Most of the variables in the data structures are not changing when the
// stream is flowing.  Any variables that do change at flow time we must
// handle with "thread-safe" care.
///////////////////////////////////////////////////////////////////////////



// App (QsApp) is the top level quickstream object.  It's a container for
// filters and streams.  Perhaps there should only be one app in a
// program, but we do not impose that.  There is no compelling reason to
// limit the number apps that a program can have.  App is used to create
// filters by loading module plugins.  App is used to create streams.
//
struct QsApp {

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
// not cause a mode switch (system call).  The man page says "Performance
// and ease-of-use of pthread_getspecific() are critical".  If they do
// cause a mode switch we need to recode this.  One can't say without
// looking at the code or running tests; for example look at how slow
// system 5 semaphores are.
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

#endif




// Stream (QsStream) is the thing the manages a group of filters and their
// flow state.  Since streams can add and remove filters when it is not
// flowing the stream needs app to be a list of loaded filters for it.
//
struct QsStream {

    struct QsApp *app;

    // maxThreads=0 means do not start any.  maxThreads does not change at
    // flow/run time, so we need no mutex to access it.
    uint32_t maxThreads; // Will not create more pthreads than this.

    uint32_t flags; // bit flags that configure the stream
    // example the bit _QS_STREAM_ALLOWLOOPS may be set to allow loops
    // in the graph.  flags does not change at flow/run time, so we need
    // no mutex to access it.

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
    pthread_cond_t cond;
    //
    // number of pthreads that exist from this stream, be they idle or
    // flowing.
    uint32_t numThreads; // must have a mutex lock to access numThreads.
    //
    // We do not need to keep list of idle threads.  We just have all idle
    // threads call pthread_cond_wait() with the above mutex and cond.
    //
    // number of thread that at in the idle thread stack.
    //
    // numThreads - numIdleThreads = "number of threads in use".
    uint32_t numIdleThreads;
    //
    // We keep a stack/pool of idle threads by having the idle threads
    // call pthread_cond_wait() with the above cond/mutex.  The running
    // threads handle themselves and we do not keep a list of running
    // pthreads.  We must have a mutex lock to access numIdleThreads and
    // numThreads.
    //
    //
    struct QsJob {

        // This will be the pthread_getspecific() data for each flow thread.
        // Each thread just calls the filter (QsFilter) input() function.
        // When there is more than on thread calling a filter input() we need to
        // know things, QsJob, about that thread in that input() call.

        // filter's who's input() is called for this job
        struct QsFilter *filter;

        // The threads must advance output buffers in the same order as the
        // input buffers, which is the order in which input() is called.
        uint32_t filterOrder;

        pthread_cond_t cond;
        pthread_mutex_t mutex;


        // The arguments to pass to the input() call:
        void **buffers;
        size_t *lens;
        bool *isFlushing;


        // The thread working on a given filter have an ID that is from a
        // sequence counter.  This threadNum (ID) is gotten from
        // filter->nextThreadNum++.  So the lowest numbered thread job for
        // this filter is the oldest, except when the counter wraps
        // through 0.  So really the oldest job is just the first in the
        // threadNum count sequence, and we must use filter->nextThreadNum
        // - filter->numThreads to get the oldest job threadNum.
        //
        // Hence, threadNum is respect to the filter.
        uint32_t threadNum;

    } *jobs; // next job that the thread should run.

    // jobs[] is an array of length maxThreads.
    //
    //
    //
    //
    ///////////////////////////////////////////////////////////////////////


    // The array list of sources is created at start:
    uint32_t numSources;       // length of sources
    // We also use sources==0 (or source != 0) as a flag that shows that
    // the stream flow-time resources have not been allocated yet (or
    // not), in place of introducing another flag.
    struct QsFilter **sources; // array of filter sources

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


    // This is the maximum number of input ports for all filters in the
    // stream.  This is computed before start.  We use it to allocate the
    // input() arguments in jobs above.
    uint32_t maxInputPorts;


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
    int (* input)(const void *buffer[], const size_t len[],
            // If isFlushing[inPort]==true is this is the last bytes of
            // data in that input port.
            const bool isFlushing[],
            uint32_t numInputs, uint32_t numOutputs);


    struct QsFilter *next; // next loaded filter in app list


    // numThreads and nextThreadNum are accessed with stream mutex locked.
    //
    // numThreads is the number of threads currently calling
    // filter->input().  nextThreadNum is the thread number of the next
    // thread to call filter->input().  The leading thread number is then
    // nextThreadNum - numThreads.  Note this work when these numbers
    // wrap through zero.  When a new thread calls filter->input() it's
    // thread number will be nextThreadNum and then we add 1 to
    // nextThreadNum.
    //
    uint32_t numThreads;    // must have stream mutex locked.
    uint32_t nextThreadNum; // must have stream mutex locked.


    ///////////////////////////////////////////////////////////////////////
    // The following are setup at stream start and cleaned up at stream
    // stop
    ///////////////////////////////////////////////////////////////////////


    // We define source as a filter with no input.  We will feed is zeros
    // when the stream is flowing.
    bool isSource;  // startup flag marking filter as a source


    // mutex used when more than one thread accesses this filters
    // outputs.
    //
    pthread_mutex_t *mutex;


    // This filter owns these output structs, in that it is the only
    // filter that may change the ring buffer pointers.
    //
    uint32_t numOutputs; // number of connected filter we write to.
    struct QsOutput *outputs; // array of struct QsOutput

    uint32_t numInputs; // number of connected input filters feeding this.


    // TODO: It'd be nice not to have this extra data.  It's not needed
    // when the stream is flowing.
    //
    // mark is extra data in this struct so that we can save a marker
    // as we look through the filters in the graph, because if the graph
    // has cycles in it, it's not easy to look at each filter just once
    // without a marker to mark a filter as looked at.
    bool mark;
};



struct QsOutput {  // points to reader filters

    // Outputs (QsOutputs) are only accessed by the filters (QSFilters)
    // that own them.

    // The "pass through" buffers are a double linked list with the "real"
    // buffer in the first one in the list.  The "real" output buffer has
    // prev==0.
    //
    // If this a "pass through" output prev points to the 
    // in another filter output that this output uses to
    // buffer its' data.
    //
    // So if prev is NOT 0 this is a "pass through" output buffer and prev
    // points toward the origin of the output thingy; if prev is 0 this is
    // not a "pass through" buffer.
    //
    struct QsOutput *prev;
    //
    // points to the next "pass through" output, if one it present; else
    // next is 0.
    struct QsOutput *next;


    // If prev is set this is not set and this output is a "pass through"
    // buffer.
    //
    struct QsBuffer *buffer;
    //
    // newBuffer is the latest created buffer and memory mapping for
    // this output.  This exists separately from buffer above for the
    // case when it is found that the size of the memory mapping was
    // not large enough in a call to qsGetOutputBuffer().
    //
    // The size of mapped memory in newBuffer should be larger than that
    // in buffer above.
    //
    // After all readPtr and writePtr values are pointing to this
    // buffer and none point to the buffer above, this newBuffer is
    // switched with buffer.
    //
    // If this newBuffer is found to be a not large enough mapping, than
    // the calls to qsGetOutputBuffer() will block until all readPtr and
    // writePtr pointers point to this newBuffer.
    //
    struct QsBuffer *newBuffer;


    // writePtr points to where to write next in mapped memory.
    uint8_t *writePtr;
    // ** Only the filter (and it's thread) that has a pointer to this
    // output can read and write this pointer, at flow time. **
    //
    struct QsReader {

        // readPtr points to a location in the mapped memory
        uint8_t *readPtr;

        // The filter that is reading.
        struct QsFilter *filter;

        // This threshold will trigger a filter->input() call, independent
        // of the other inputs.
        //
        // The filter->input() may just return without advancing any input
        // or output, effectively ignoring the input() call like the
        // threshold trigger conditions where not reached.  In this way
        // we may have arbitrarily complex threshold trigger conditions.
        size_t thresholdLength;

        // The input port number that the filter being written to sees in
        // it's input(,,portNum,) call.
        uint32_t inputPortNum;

    } *readers; // array of filter readers

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


// The filter that is having construct() or destroy() called.  There is
// only one thread when they are called.
//
// TODO: This makes the API not thread-safe.
extern
struct QsFilter *_qsConstructFilter;

// The filter that is having start() or stop() called.  There is only one
// thread when they are called.
//
// TODO: This makes the API not thread-safe.
extern
struct QsFilter *_qsStartFilter;



// These below functions are not API user interfaces:'

extern
uint32_t nThreadFlow(struct QsStream *s);


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



#if 0 // Not needed yet.
// Set filter->mark = val for every filter in the app.
static inline
void AppSetFilterMarks(struct QsApp *app, bool val) {
    for(struct QsFilter *f=app->filters; f; f=f->next)
        f->mark = val;
}
#endif

// Set filter->mark = val for every filter in just this stream.
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
