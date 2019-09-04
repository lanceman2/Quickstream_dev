/* This is a private header file that is only used in the quickstream code
 * internal to libquickstream.so.  The libquickstream.so user, in general,
 * should not include this header file.  Hence this file is not installed.
 * */

// This file is the guts of quickstream libquickstream API.  By
// understanding each data member in these data structures you can get a
// clear understanding of the insides of quickstream, and code it.  But
// it's likely that you will need to "scope the code" in app.c,
// filter.c, stream.c, thread.c, and process.c to get there.


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


struct QsStream {

    struct QsApp *app;

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


struct QsOutput {  // reader

    // The "reading filter" (or access filter) that sees this output as
    // input.
    struct QsFilter *filter;

    // Here's where it gets weird: We need a buffer and a write pointer,
    // where the buffer may be shared between outputs in the same filter
    // and shared between outputs in different adjacent filters.
    //
    // That's because we want to be able to have pass-through buffers that
    // use one circular buffer that is passed through filters (and maybe
    // changing the values in the memory) without a memory copy.
    //
    // The writer for this particular output.  This writer can be shared
    // between many outputs in one filter, hence this is just a pointer
    // and a single writer is used for more than one output.
    //
    struct QsWriter *writer;

    // read filter access pointer to the circular buffer.
    //
    uint8_t *accessPtr;


    // All these limits may be set in the reading filters start()
    // function.
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
        // input() call.
};


struct QsWriter {

    size_t maxWrite; // maximum the writer can write.  If it write more
    // than this then the memory could be corrupted.

    // QsBuffer may be shared by other QsWriter's, if the buffer is shared
    // it's a pass-through buffer, and it should be also written to in an
    // neighboring down-stream filter.
    struct QsBuffer *buffer;

    // This is the last place a writing filter wrote to in this memory.
    uint8_t *writePtr;
};


struct QsBuffer {

    // They can be many outputs and filters accessing this memory, unlike
    // GNU radio.

    uint8_t *mem; // Pointer to start of mmap()ed memory.

    // These two parameters make it a circular buffer or ring buffer.  See
    // makeRingBuffer.c.
    size_t mapLength, overhangLength;
};



// The current filter that is having its' input() called in this thread.
extern
__thread struct QsFilter *_qsCurrentFilter;



// These below functions are not API user interfaces:


extern
void AllocateRingBuffers(struct QsFilter *f);


extern
void FreeRingBuffers(struct QsFilter *f);

extern
int stream_run_0p_0t(struct QsStream *s);


extern
void *makeRingBuffer(size_t *len, size_t *overhang);


extern
void freeRingBuffer(void *x, size_t len, size_t overhang);


