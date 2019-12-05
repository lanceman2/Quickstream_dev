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


// https://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Atomic-Builtins.html
// example: _sync_fetch_and_add()
// Or use:
// #include <stdatomic.h>
// example: https://en.cppreference.com/w/c/language/atomic


// QsThread is only in the QsStream struct
//
struct QsThread {

    pthread_t pthread;
    struct QsThread *next;
};


//#define QS_STREAM_DEFAULTMAXTHTREADS (8)
#define QS_STREAM_DEFAULTMAXTHTREADS (0)

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
#define QS_MAX_CHANNELS              ((uint32_t) (128))

// So we don't run away mapping to much memory for ring-buffers.  There
// just has to be some limit.
#define QS_MAX_BUFFERLEN             ((size_t) (16*4*1024))



///////////////////////////////////////////////////////////////////////////
//                 THREAD SAFETY WARNING
//
// Most of the variables in the data structures are not changing when the
// stream is flowing.  Any variables that do change at flow time we must
// handle with "thread-safe" care.
///////////////////////////////////////////////////////////////////////////


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
    //
    // number of pthreads that exist from this stream, be they idle or
    // flowing.
    uint32_t numThreads; // must have a mutex lock to access numThreads.
    // number of thread that at in the idle thread stack.
    uint32_t numIdleThreads;
    //
    // We keep a stack/pool of idle threads in idleThreads.  The running
    // threads handle themselves and we do not keep a list of running
    // pthreads.  We must have a mutex lock to access idleThreads.
    struct QsThread *idleThreads;
    //
    ///////////////////////////////////////////////////////////////////////


    // The array list of sources is created at start:
    uint32_t numSources;       // length of sources
    // We also use sources==0 (or source != 0) as a flag that shows that
    // the stream flow-time resources have not been allocated yet (or
    // not), in place of introducing another flag.
    struct QsFilter **sources; // array of filter sources

    // This list of filter connections is not used while the stream is
    // running.  It's queried a stream start, and the QsFilter data
    // structs are setup at startup.  The QsFilter data structures are
    // used when the stream is running.
    //
    // tallied filter connections:
    uint32_t numConnections;// length of "from" and "to" arrays
    struct QsFilter **from; // array of filter pointers
    struct QsFilter **to;   // array of filter pointers

    struct QsStream *next; // next stream in app list of streams
};


struct QsOutput;

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
            const uint32_t isFlushing[],
            uint32_t numInputs, uint32_t numOutputs);


    struct QsFilter *next; // next loaded filter in app list

    // Set to true if this filter input() can be called by more than one
    // thread at a time.
    //
    bool isThreadSafe;


    ///////////////////////////////////////////////////////////////////////
    // The following are setup at stream start and cleaned up at stream
    // stop
    ///////////////////////////////////////////////////////////////////////


    // We define source as a filter with no input.  We will feed is zeros
    // when the stream is flowing.
    bool isSource;  // startup flag marking filter as a source


    // This filter owns these output structs, in that it is the only
    // filter that may change the ring buffer pointers.
    //
    uint32_t numOutputs; // number of connected filter we write to.
    struct QsOutput *outputs; // array of struct QsOutput

    uint32_t numInputs; // number of connected input filters feeding this.


    // TODO: It'd be nice not to have this extra data. It's not needed
    // when the stream is flowing.
    //
    // mark is extra data in this struct so that we can save a marker
    // as we look through the filters in the graph, because if the graph
    // has cycles in it, it's not easy to look at each filter just once
    // without a marker to mark a filter as looked at.
    bool mark;
};



struct QsOutput {  // points to reader filters

    // readPtr and writer->writePtr are the two only values in this struct
    // that can change while the stream is in the flowing state.

    // The "reading filter" (or access filter) that sees this output as
    // input.
    struct QsFilter *filter;

    // The input port number that the filter being written to sees
    // in it's input(,,portNum,) call.
    uint32_t inputPortNum;

    // ** Only the filter (and it's thread) that has a pointer to this
    // output can read and write to the writePtr in this writer, at flow
    // time.
    // **
    // 
    struct QsBuffer *buffer;


    // read filter access pointer to the circular buffer.
    //
    // ** Only the filter (and it's thread) that has a pointer to this
    // output can read and write this pointer, at flow time. **
    //
    uint8_t *readPtr;


    // input() just returns 0 if a threshold is not reached.


    // maxInput may be set in the reading filters start() function
    // and does not change until the next start().
    size_t
        maxInput; // = 0 by default.  This reading filter will not read
        // (input) more than this, if this is set.  The filter sets this
        // so that the stream running does not call input() with more data
        // than this.  This is a convenience, so the filter does not need
        // to tell the stream running to not advance the buffer so far at
        // every input() call had the input buffer length exceeded this
        // number.  When there is much more readable data than this, the
        // read filter input() will be called many times, with buffer read
        // length values of maxInput or less.
};



struct QsBuffer {  // all outputs have a circular buffer

    // Their can be many outputs pointing to this (using this) buffer.

    // This is the next place a writing filter will write to in this
    // memory.
    uint8_t *writePtr;

    // 0 terminated array of pointers to outputs
    // 
    // Example getting readPtr: ptr = outputs[i]->readPtr
    // 
    // Just a little faster access to the outputs that use this buffer.
    struct QsOutputs **outputs;

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
    //
    uint32_t refCount; // Number of writers pointing to this.
};



//
// TODO: If an app makes a lot of threads that are not related to
// quickstream, this could be wasteful, so we may want to allocate this.
// Maybe these can be part of QsThread.
//
//
// The current thread that corresponds with the filter who's input() is
// being called now.
extern
__thread struct QsInput {

    size_t len;
    bool advanceInputs_wasCalled;
    struct QsFilter *filter; // filter module having input() called.

} _input;



// The filter that is having construct(), start(), stop() or destroy()
// called.  There is only one thread when they are called.  TODO: This
// makes the API not thread-safe.
extern
struct QsFilter *_qsCurrentFilter;



// These below functions are not API user interfaces:


extern
void AllocateRingBuffers(struct QsFilter *f);


extern
void FreeRingBuffersAndWriters(struct QsFilter *f);

extern
int stream_run_0p_0t(struct QsStream *s);


extern
void *makeRingBuffer(size_t *len, size_t *overhang);


extern
void freeRingBuffer(void *x, size_t len, size_t overhang);


extern
void setupSendOutput(struct QsFilter *f);


extern
void unsetupSendOutput(struct QsFilter *f);


extern
void _qsOutput(size_t len, uint32_t outputPortNum);


extern
size_t GetReadLength(struct QsOutput *output);


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
            s->to[i]->mark = val;
        return;
    }

    // Case where the stream is not setup to flow yet, so we have not
    // found the source filters (sources) yet.  The filters are just
    // listed in the stream connection arrays, to[], and from[].
    for(uint32_t i=0; i<s->numConnections; ++i) {
        s->to[i]->mark = val;
        s->from[i]->mark = val;
    }
}
