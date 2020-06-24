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
// time.  And the same goes for ASSERT(); ASSERT() is not a debugging
// thing either, it is the coder being to lazy to recover from a large
// number of failure code paths.  If malloc(10) fails we call ASSERT.
// If pthread_mutex_lock() fails we call ASSERT. ...
//
#define CHECK(x) \
    do { \
        int ret = (x); \
        ASSERT(ret == 0, #x "=%d FAILED", ret); \
    } while(0)




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


// We must limit the length of the filter name, so that we can use it in
// the code without too much memory.
//
#define _QS_FILTER_MAXNAMELEN     ((size_t) 125)



// We reuse the filter mark as a flag when we are in the construct() and
// destroy() calls.  So long as the values unique that should be okay.
// Same idea for start() and stop().
//
// For in construct() filter->mark == _QS_IN_CONSTRUCT
// For in destroy()   filter->mark == _QS_IN_DESTROY
//
// Getting random numbers, run:
//
//   cat SOME_FILES | sha512sum -
// 
// and copy and paste a random chunk of 8 characters in that or run:
//
//   od -An -tx -N 4 < /dev/random
//
// or flip a coin for an hour.  TODO: Maybe prime and random is better.
//
//
// These values are just fixed and random
//
//
// We set filter mark to this when calling the corresponding filter
// functions:
//
// construct()
#define _QS_IN_CONSTRUCT        ((uint32_t) 0xe583e10c)
// and destroy()
#define _QS_IN_DESTROY          ((uint32_t) 0x17fb51d9)

// Sets filter mark when in filter start()
#define _QS_IN_START            ((uint32_t) 0xfa74ac1c)
// Sets filter mark when in filter stop()
#define _QS_IN_STOP             ((uint32_t) 0x091ba4df)

// We set the job magic number when in filter input()
#define _QS_IS_JOB              ((uint32_t) 0x38def4de)



// We set controller mark to this when calling the corresponding
// controller functions:
// 
// construct()
#define _QS_IN_CCONSTRUCT      ((uint32_t) 0x540e2c1f)
// and destroy()
#define _QS_IN_CDESTROY        ((uint32_t) 0x314fad5d)

// Sets controller mark when in controller preStart()
#define _QS_IN_PRESTART        ((uint32_t) 0xa74ac1c3)
// Sets controller mark when in controller preStop()
#define _QS_IN_PRESTOP         ((uint32_t) 0x91ba4d3c)

// Sets controller mark when in controller postStart()
#define _QS_IN_POSTSTART       ((uint32_t) 0xbec11df1)
// Sets controller mark when in controller postStop()
#define _QS_IN_POSTSTOP        ((uint32_t) 0x55cef7f7)


#define _QS_STREAM_TYPE        ((uint32_t) 0x17715c6c)
#define _QS_APP_TYPE           ((uint32_t) 0xc70ec703)



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

extern uint32_t _qsAppCount; // always increasing.

struct QsDictionary;
struct QsController;
struct QsApp;


struct QsScriptControllerLoader {

    // Example: A DSO (dynamic shared object) that loads python
    // scripts.  When the stream flows that scripts run as controllers,
    // like the C/C++ controllers, but they are script (python) wrappers.
    void *dlhandle;

    void *(*loadControllerScript)(
            const char *path, int argc, const char **argv);

    char *scriptName;
    char *scriptSuffix;
};


// App (QsApp) is the top level quickstream object.  It's a container for
// streams.  Perhaps there should only be one app in a program, but we do
// not impose that.  There is no compelling reason to limit the number
// apps that a program can have.  App is used to create streams.  App may
// also contain default settings that can be used by the streams.
//
// App really need to exist as a top level container/manager for all
// things quickstream.
//
struct QsApp {

    // type of struct.  Must be _QS_APP_TYPE.
    uint32_t type;

    // We get id from _qsAppCount just before this app is created.
    uint32_t id; // The app ID.  Unique to a given process.

    // streamCount is always increasing.  streamCount is used to get
    // stream IDs and controller IDs.
    uint32_t streamCount;

    // A list of script controllers that run scripting languages like
    // python or lua.  It's a list of struct QsScriptControllerLoader.
    //
    struct QsDictionary *scriptControllerLoaders;

    // List of controllers keyed by name.  TODO: This dictionary list may
    // not be needed.
    //
    struct QsDictionary *controllers;
    //
    // And a doubly linked list of controllers, so we have an order in
    // which they are loaded, so we may call preStart() and postStart() in
    // load order, and preStop() and postStop() in reverse load order.
    struct QsController *first; // first loaded
    struct QsController *last; // last loaded


    // We could have more than one stream.  We can't delete or edit one
    // while it is running.  You could do something weird like configure
    // one stream while another stream is running.  We can also run more
    // than one stream at a time; we have a test that does that.
    //
    // List of streams.  Head of a singly linked list.
    struct QsStream *streams;
};


// For pthread_getspecific() and pthread_setspecific().  One hopes that
// pthread_getspecific() and pthread_setspecific() are very fast and do
// not cause a mode switch (system call) each time they are called.  The
// man page says "Performance and ease-of-use of pthread_getspecific() are
// critical".  If they do cause a mode switch we need to recode this.  One
// can't say without looking at the code or running tests; as an example
// of how shitty code got into the main Linux Kernel, look at system 5
// semaphores: they are very slow and every call in the API incurs a
// system call.
//
// We use _qsKey for passing data to calls to the filter API from filter
// input() functions and other filter calls.
//
// The thread specific data from this key is either a QsJob or a
// QsFilter.
//
extern
pthread_key_t _qsKey;


// For thread specific data for getting the controller object in
// controller modules:
extern
pthread_key_t _qsControllerKey;


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
// This kind of thing may be needed for non-debug building in the
// future.
#endif


struct QsFilter;



struct QsController {

    // The order to mark and then parameters must be the same in the
    // struct QsFilter 

    // First in struct
    uint32_t mark;

    // parameters should be second in this struct.
    // List of parameters that are owned by this controller.
    struct QsDictionary *parameters;

    // Controller modules are loaded in a similar way filter modules are
    // loaded as plugin modules, but they are listed in the App which
    // gives them a view of all streams and all filters in them.
    // Controllers mess with filter parameters.  A single controller
    // module can access all filters and all parameters in the said
    // filters.

    void *dlhandle; // from dlopen()

    struct QsApp *app;

    // malloc() allocated unique name for this controller in this app.
    char *name;

    // For the doubly linked list that is in App.
    struct QsController *next, *prev;

    // Callback functions that may be loaded.  We do not get a copy of the
    // construct() and destroy() functions because they are only called
    // once, so we just dlsym() (if we have a dlhandle) them just before
    // we call them.

    int (*preStart)(struct QsStream *stream, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs);
    int (*postStart)(struct QsStream *stream, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs);

    int (*preStop)(struct QsStream *stream, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs);
    int (*postStop)(struct QsStream *stream, struct QsFilter *f,
        uint32_t numInputs, uint32_t numOutputs);
};



// Stream (QsStream) is the thing the manages a group of filters and their
// flow state.
//
struct QsStream {

    // type must be first in this struct.
    // Must be _QS_STREAM_TYPE.
    // The type of struct this is:
    uint32_t type;

    // We can have many streams in an app (QsApp).
    //
    struct QsApp *app;

    // id from app streamCount.
    uint32_t id;

    // Used to signal to stop source input() calls.
    // We need a stream mutex to access isRunning or we need to make this
    // atomic_int via #include <stdatomic.h>
    //
    // We want the use to be able to set this in a signal handler so to
    // not deadlock a mutex we really need atomic setting and getting.
    atomic_int isSourcing;

    // A Dictionary list of the filters keyed by filter name.
    struct QsDictionary *dict;

    // List of all filters that this stream can use.  Head of a singly
    // linked list.
    struct QsFilter *filters;

    // Allocated pthread_t array that is of length maxThreads.
    pthread_t *threads;
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
    // This will be equal to numThreads after the thread gets past the
    // first STREAM LOCK CHECK(pthread_mutex_lock(&s->mutex));
    //
    uint32_t numWorkerThreads;
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
    // qsStreamConnectFilters() is not called at flow time.
    //
    // tallied filter connections from qsStreamConnectFilters():
    struct QsConnection {

        struct QsFilter *from; // from filter
        struct QsFilter *to;   // to filter

        uint32_t fromPortNum; // output port number for from filter
        uint32_t toPortNum;   // input port number for to filter

    } *connections; // malloc()ed array of connections


    uint32_t numConnections;// length of connections array

    // TODO: We may not need next; we could use app::dict instead.
    //
    // A singly linked list of stream that starts at app.
    struct QsStream *next; // next stream in app list of streams
};


// The filter (QsFilter) is loaded by app as a module DSO (dynamic shared
// object) plugin.  The filter after be loaded is added to a only one
// stream at a time.  If the filter is added to another stream, it will be
// removed from the previous stream; so ya, a filter can be only in one
// stream at a time.  When finished with a filter, the filter is unloaded
// by it's app.  quickstream users can write filters using
// include/quickstream/filter.h.  The quickstream software package also
// comes with a large selection of filters.
//
struct QsFilter {

    // mark is extra data in this struct so that we can save a marker as
    // we look through the filters in the graph, because if the graph has
    // cycles in it, it's not easy to look at each filter just once
    // without a marker to mark a filter as looked at.
    //
    // This mark is used for different things at different stages of the
    // flow stream.  It's just a multipurpose flag.  In some uses it's a
    // magic number, so it's at the top of the structure.
    //
    // At flow time mark is used to mark that input() has finished being
    // called, and the stream mutex is used to access it.
    //
    // TODO: make mark a union with descriptive names for the different
    // uses.
    //
    // mark must be first in this struct
    //
    uint32_t mark;

    // parameters should be second in this struct.
    // List of Parameters that this filter owns:
    struct QsDictionary *parameters;

    // list of controller's qsAddPreFilterInput() callbacks
    struct QsDictionary *preInputCallbacks;

    // List of controller's qsAddPostFilterInput() callbacks
    struct QsDictionary *postInputCallbacks;


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


    struct QsFilter *next; // next loaded filter in the stream filter list


    ///////////////////////////////////////////////////////////////////////
    struct QsJob {


        // magic must be at the top of this struct
        //
        // It must lineup with the mark in QsFilter.
        //
        uint32_t magic; // is set to _QS_IS_JOB when it's valid.


        ///////////////// STREAM MUTEX GROUP ////////////////////////////
        //
        // to put in filter unused stack or stream queue
        struct QsJob *next;
        // prev is for when we need this in the filter workingQueue
        // because the workingQueue can have jobs finish out of order some
        // times.
        struct QsJob *prev;
        //
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
        // outputLens from qsOuput() and qsGetOutputBuffer() calls from in
        // filter input().   Length of this array is filter numOutputs.
        size_t *outputLens; // amount output was advanced in input() call.

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
    struct QsReader {
        //
        // readPtr points to a location in the mapped memory, "ring
        // buffer".
        //
        // After initialization, readPtr is only read and
        // written by the reading filter.  We use a reading filter mutex
        // for multi-thread filters input()s, but otherwise
        // reading/writing the buffer is lock-less.
        //
        uint8_t *readPtr;

        // readLength is the number of bytes to the write pointer at this
        // pass-through level.  Accessing readLength requires a stream
        // mutex lock.
        size_t readLength;

        // The filter that is reading.
        struct QsFilter *filter;

        // feedFilter is the filter that is writing to this reader.
        struct QsFilter *feedFilter;

        struct QsBuffer *buffer;

        // This threshold will trigger a filter->input() call, independent
        // of the other inputs.
        //
        // The filter->input() may just return without advancing any input
        // or output, effectively ignoring the input() call like the
        // threshold trigger conditions where not reached.  In this way
        // we may have arbitrarily complex threshold trigger conditions.
        size_t threshold; // Length in bytes to cause input() to be called.

        // The reading filter promises to read some input data so long as
        // the buffer input length >= maxRead.
        //
        // It only has to read 1 byte to fulfill this promise, but so long
        // as the readable amount of data on this port is >= maxRead is
        // will keep having it's input() called until the readable amount
        // of data on this port is < maxRead or an output buffer write
        // pointer is at a limit (???).
        //
        // This parameter guarantees that we can calculate a fixed ring
        // buffer size that will not be overrun.
        //
        size_t maxRead; // Length in bytes.

        // The input port number that this filter being written to sees in
        // it's input(,,portNum,) call.
        uint32_t inputPortNum;
    }
    // array of pointers to readers array that is in feed filters
    // and not all the feed filters are the same filter.
    //
    // Indexed by input port number 0, 1, 2, 3, ..., numInputs-1
    **readers;
    //
    /////////////////////////////////////////////////////////////////////


    ///////////////// STREAM MUTEX GROUP ////////////////////////////////
    //
    //
    //   See the "job flow graph" by running (bash command-line):
    //
    //
    //      display ../share/doc/quickstream/jobFlow.dot
    //
    //
    //
    // All jobs in this filter are in one of these three job lists, or are
    // running with threads in the stream.
    //
    // 1. stream job queue - waiting for a worker thread
    //
    // 2. working queue - 0 to maxThreads that can be working on the jobs
    //
    // 3. unused - a stack of job memory, for when the memory is not used
    //             in the working queue or stream job queue.
    //
    //  maxThreads = number in working queue + number in unused queue.
    //
    //struct QsJob *stage;  // queue of one job that is being built.
    struct QsJob *unused; // stack of 0 or more unused jobs.
    //
    // If stream->maxThread == 0 then jobs will not be used and buffers,
    // lens, and isFlushing will be allocated on the stack that is running
    // this flow.
    //
    // We exclude the case (filter) maxThreads=0 and use maxThreads=1.
    //
    // At flow time, array "jobs" has length maxThreads+1 unless the
    // stream total maxThreads + 1 is less than that.  See
    // GetNumAllocJobsForFilter() in this file.  The + 1 is from needing
    // one the filters jobs in the filter stage at all times.
    //
    uint32_t maxThreads; // per this filter.
    //
    //
    // numWorkingThreads, workingLast, and workingFirst are accessed
    // with stream mutex locked.  They make the data for the filters
    // working queue.  numWorkingThreads is the number of threads working,
    // calling filter input(), and is the number in the working thread
    // (job) queue.  This working queue is doubly linked because jobs
    // may be removed from any point in the line, just because they had
    // faster worker threads and their was nothing causing the job to stay
    // in order, like not having input or output data accessed.
    // The filter's working thread queue is needed so we know the order
    // of the in which the working thread will access the buffers.  If
    // this order was not needed to be known we would not need this queue
    // to exist, and the thread's stack would hold the job until it puts
    // it into the filter's unused stack.
    //
    // Note: filter maxThreads is the maximum number of working threads so
    // long as it is less than the streams maxThreads.  See
    // GetNumAllocJobsForFilter() below.
    //
    uint32_t numWorkingThreads; // number of threads working for the filter
    struct QsJob *workingFirst; // First in thread working queue
    struct QsJob *workingLast;  // Last in thread working queue
    //
    //
    /////////////////////////////////////////////////////////////////////
 

    // We define source as a filter with no input.  We will feed is zeros
    // when the stream is flowing.
    bool isSource; // startup flag marking filter as a source
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

    // writePtr points to where to write next in mapped memory. 
    //
    // writePtr can only be read from and written to by the filter that
    // feeds this output.  If the filter that owns this output can run
    // input() in multiple threads a filter mutex lock is required to read
    // or write to this writePtr, but otherwise this is a lock-less
    // buffer when in the input() call.
    //
    uint8_t *writePtr;

    // The filter that owns this output promises to not write more than
    // maxWrite bytes to the buffer.
    //
    size_t maxWrite;

    // This is the maximum of maxWrite and all reader maxRead for
    // this output level in the pass-through buffer list.
    //
    // See the function: GetMappingLengths() in buffer.c
    //
    // This is used to calculate the ring buffer size and than
    // is used to determine if writing to the buffer is blocked
    // by buffer being full.
    size_t maxLength;

    // The number of bytes written in the last write, qsOutput().
    //size_t advanceLength;  this is now in job::outputLens[]
    // because the filter that is owns the output and writePtr could be
    // multi-threaded which means that with will be many of these
    // counters.


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
    // This structure stays constant while the stream is flowing.
    //
    // There are two adjacent memory mappings per buffer.
    //
    // The start of the first memory mapping is at end + mapLength.  We
    // save "end" in the data structure because we use it more in looping
    // calculations than the "start" of the memory.
    //
    uint8_t *end; // Pointer to end of the first mmap()ed memory.

    //
    // These two elements make it a circular buffer or ring buffer.  See
    // makeRingBuffer.c.  The mapLength is the most the buffer can hold to
    // be read by a filter.  The overhangLength is the length of the
    // "wrap" mapping that is a second memory mapping just after the
    // mapLength mapping, and is the most that can be written or read in
    // one filter transfer "operation".
    //
    size_t mapLength, overhangLength; // in bytes.
};




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



extern
int StreamRemoveFilterConnections(struct QsStream *s, struct QsFilter *f);


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
        for(uint32_t i=s->numSources-1; i!=-1; --i)
            s->sources[i]->mark = val;
        for(uint32_t i=s->numConnections-1; i!=-1; --i)
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


// This returns the number of jobs that will be allocated for a filter
// after filter start() and before filter stop().
//
// Note: It depends on the maxThreads in the filter and the maxThreads in
// the whole stream, depending on which is larger.
//
// We do this instead of; adding another int type in the data structures,
// or repeating the code block in this function.
static inline
uint32_t
GetNumAllocJobsForFilter(struct QsStream *s, struct QsFilter *f) {
    DASSERT(s);
    DASSERT(f);
    DASSERT(s == f->stream);

    uint32_t numJobs = f->maxThreads;
    if(s->maxThreads && numJobs > f->stream->maxThreads)
        // We do not need to have more jobs than this:
        numJobs = f->stream->maxThreads;

    return numJobs;
}


struct QsWorkPermit {

    // The stream that the thread will work for
    struct QsStream *stream;

    // This ID is from counting the number of threads that the stream has
    // working for it.  id starts at 1.
    uint32_t id;
};


extern
void *RunningWorkerThread(struct QsWorkPermit *p);


static inline
void LaunchWorkerThread(struct QsStream *s) {

    // Stream does not have its' quota of worker threads.
    DASSERT(s->numThreads < s->maxThreads);
    struct QsWorkPermit *p = malloc(sizeof(*p));
    ASSERT(p, "malloc(%zu) failed", sizeof(*p));
    p->stream = s;
    p->id = (++s->numThreads);
    CHECK(pthread_create(&s->threads[p->id-1], 0/*attr*/,
            (void *(*) (void *)) RunningWorkerThread, p));
    // RunningWorkerThread() will free p.

    // We'll let pthread memory automatically be freed when
    // they return from RunningWorkerThread().
    CHECK(pthread_detach(s->threads[p->id-1]));
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

extern
struct QsDictionary *GetStreamDictionary(const struct QsStream *s);


// Just Frees the malloc allocated memory that is pointed to from the
// Parameter Dictionary.  The Parameter Dictionary is not destroyed with
// this.
//
extern
void
_qsParametersDictionaryDestory(struct QsStream *s);


// filters and controllers are in this relative directory:
#define MOD_PREFIX "/lib/quickstream/plugins/"
// Portability
#define DIR_CHAR '/'
#define DIR_STR  "/"


// The returned string must be free()ed or returns 0 if path is not found.
// Used to find module files of different types.
extern
char *GetPluginPath(const char *prefix, const char *category,
        const char *name, const char *suffix);
