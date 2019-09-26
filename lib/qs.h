/* This is a private header file that is only used in the quickstream code
 * internal to libquickstream.so.  The libquickstream.so user, in general,
 * should not include this header file.  Hence this file is not installed
 * and is kept with the lib/libquickstream.so source code.
 */


// This file is the guts of quickstream.  By understanding each data
// member in these data structures you can get a clear understanding of
// the insides of quickstream, and code it.  Easier said than done...



// App (QsApp) is the top level quickstream object.  It's a container for
// filters and streams.  Perhaps there should only be one app in a
// program, but we do not impose that.  There is no compelling reason to
// limit the number apps that a program can have.  App is used to create
// filters by loading module plugins.  App is used to create streams.
//
struct QsApp {

    // List of filters.  Head of a singly linked list.
    struct QsFilter *filters;

    // We could have more than one stream, but we can't delete or edit one
    // while it is running.  You could do something weird like configure
    // one stream while another stream is running.
    //
    // List of streams.  Head of a singly linked list.
    struct QsStream *streams;
};


struct QsThread {

    struct QsStream *stream;  // stream that owns this thread
    struct QSProcess *process;
};


struct QsProcess {

    struct QsStream *stream; // stream that owns this process

    uint32_t numThreads;      // length of threads
    struct QsThread *threads; // array of threads
};



// bit Flags for the stream
//
#define _QS_STREAM_ALLOWLOOPS     (01)

#define _QS_STREAM_DEFAULTFLAGS   (0)



// Stream (QsStream) is the thing the manages a group of filters and their
// flow state.  Since streams can add and remove filters when it is not
// flowing the stream needs app to be a list of loaded filters for it.
//
struct QsStream {

    struct QsApp *app;

    uint32_t flags; // bit flags that configure the stream
    // example the bit _QS_STREAM_ALLOWLOOPS may be set to allow loops
    // in the graph.

    // keeps state that it passes to filter input()
    uint32_t flowState;

    uint32_t numThreads;       // length of threads
    struct QsThread **threads; // array of threads

    // The array list of sources is created at start:
    uint32_t numSources;       // length of sources
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

    struct QsStream *next; // next stream in app list
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
    struct QsThread *thread; // thread that this filter will run in

    // name never changes after filter loading/creation pointer to
    // malloc()ed memory.
    char *name; // unique name for the Filter in a given app

    // Callback functions that may be loaded.  We do not get a copy of
    // the construct() and destroy() functions because they are only
    // called once, so we just dlsym() (if we have a dlhandle) them just
    // before we call them.
    int (* start)(uint32_t numInputs, uint32_t numOutputs);
    int (* stop)(uint32_t numInputs, uint32_t numOutputs);
    int (* input)(void *buffer, size_t len, uint32_t inputChannelNum);

    struct QsFilter *next; // next loaded filter in app list


    ///////////////////////////////////////////////////////////////////////
    // The following are setup at stream start and cleaned up at stream
    // stop
    ///////////////////////////////////////////////////////////////////////
    //
    // We use this u variable at startup and at runtime for two different
    // things at two different times in the code.
    union {
        // We don't used both of these 2 variables at the same time so
        // they can use the same memory:
        uint32_t numInputs; // runtime number of connected input filters
        uint32_t isSource;  // startup flag marking filter as a source
    } u;

    //
    uint32_t numOutputs; // number of connected output filters
    struct QsOutput *outputs; // array of struct QsOutput
};


// Given the parameters in these data structures (QsOutput and QsBuffer)
// the stream running code should be able to determine the size needed for
// the associated circular buffers.
//
// QsOutput is the data for a reader filter that another filter feeds
// data.
//
// We cannot put all the ring buffer data in one data structure because
// the ring buffer can be configured/shared between filters in different
// ways.


#define _QS_DEFAULT_maxReadThreshold  ((size_t) 1024)
#define _QS_DEFAULT_minReadThreshold  ((size_t) 1)
#define _QS_DEFAULT_maxReadSize       ((size_t) 0) // not set


struct QsOutput {  // points to reader filters

    // The "reading filter" (or access filter) that sees this output as
    // input.
    struct QsFilter *filter;

    // Here's where it gets weird: We need a buffer and a write pointer,
    // where the buffer may be shared between outputs in the same filter
    // and shared between outputs in different adjacent filters.
    // So we may have more than one output point to a writer.
    //
    // The writer for this particular output.  This writer can be shared
    // between many outputs in one filter, hence this is just a pointer
    // and a single writer that may be used for more than one output.
    //
    // TODO: "pass-through buffers" in which the buffer can be shared
    // for multiple filter levels, that is a filter can read the buffer
    // and then treat it like it is the writer to the next filter.
    // The passing filter can't change the size of the buffer, but it
    // can over-write it's content if it's careful.
    //
    struct QsWriter *writer;

    // read filter access pointer to the circular buffer.
    //
    uint8_t *readPtr;


    // All these limits may be set in the reading filters start()
    // function.
    //
    // Sizes in bytes that may be set at filter start():
    size_t
        maxReadThreshold, // This reading filter promises to read
        // any data at or above this threshold; so we will keep calling
        // the filter input() function until the amount that can be read
        // is less than this threshold.

        minReadThreshold, // This reading filter will not read
        // any data until this threshold is met; so we will not call the
        // filter input() function until this threshold is met.

        maxReadSize; // This reading filter will not read more than
        // this, if this is set.  The filter sets this so that the stream
        // running does not call input() with more data than this.  This
        // is a convenience, so the filter does not need to tell the
        // stream running to not advance the buffer so far at every
        // input() call had the input length exceeded this number.
};


struct QsWriter {  // all outputs need a writer

    // There can be many outputs pointing to each writer.

    size_t maxWrite; // maximum the writer can write.  If it writes more
    // than this then the memory could be overrun.

    // QsBuffer may be shared by other QsWriter's, if the buffer is shared
    // it's a pass-through buffer, and it should be also written to in an
    // neighboring down-stream filter.
    struct QsBuffer *buffer;

    // This is the last place a writing filter wrote to in this memory.
    uint8_t *writePtr;

    uint32_t refCount; // Number of outputs pointing to this.
};


struct QsBuffer {  // all writers need a buffer

    // Their can be many writers pointing to this buffer.

    uint8_t *mem; // Pointer to start of mmap()ed memory.

    // These two parameters make it a circular buffer or ring buffer.  See
    // makeRingBuffer.c.
    size_t mapLength, overhangLength;

    uint32_t refCount; // Number of writers pointing to this.
};



// The current filter that is having its' input() called in this thread.
//
// TODO: If an app makes a lot of threads that are not related to
// quickstream, this could be wasteful, so we may want to allocate this.
extern
__thread struct QsFilter *_qsInputFilter;



// The filter that is having start() called.
// There is only one thread when start() is called.
extern
struct QsFilter *_qsStartFilter;




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


