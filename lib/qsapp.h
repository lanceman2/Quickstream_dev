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

    // List of streams.  Head of a singly linked list.
    struct QsStream *streams;

    // List of processes
    //struct QsProcess *processes;
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

    // filter connections:
    uint32_t numConnections;// length of from and to
    struct QsFilter **from; // array of filter pointers
    struct QsFilter **to;   // array of filter pointers

    struct QsStream *next; // next stream in app list
};


struct QsFilter {

    void *dlhandle; // from dlopen()

    struct QsApp *app;       // This does not change
    struct QsStream *stream; // This stream can be changed
    struct QsThread *thread; // thread that this filter will run in

    // The name never changes after filter loading/creation
    // pointer to malloc()ed memory.
    char *name; // unique name for the Filter in a given app

    // Callback functions that may be loaded.  We don not get a copy of
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


struct QsOutput {

    // The "reading filter" (or access filter) that sees this output as
    // input.
    struct QsFilter *filter;

    // Here's where it gets weird: We need a buffer and a write pointer,
    // where the buffer can be shared between filters.  That's because we
    // want to be able to have pass-through buffers that use one circular
    // buffer that is passed through filters (and maybe changing the
    // values in the memory) without a memory copy.
    struct QsWriter *writer;

    // This is where the filter that is reading (and maybe writing) the
    // buffer last access in the buffer memory.
    //
    // TODO: in the case of a pass-through buffer this should also be the
    // same as a writePtr in a struct QsWriter.  So that sucks having to
    // keep this pointer and the QsWriter::writePtr with the same value.
    uint8_t *accessPtr;

    // Sizes in bytes:
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

    // QsBuffer may be shared by other QsWriter's, if the buffer is shared
    // it's a pass-through buffer, and it should be also written to in an
    // neighboring down-stream filter.
    struct QsBuffer *buffer;

    // This is the last place a writing filter wrote to in this memory.
    uint8_t *writePtr;
};


struct QsBuffer {

    uint8_t *mem; // Pointer to start of mmap()ed memory.

    // These two parameters make it a circular buffer.  See
    // makeRingBuffer.c.
    size_t mapLength, overhangLength;
};



extern
__thread struct QsFilter *_qsCurrentFilter;


extern
void AllocateOutputBuffers(struct QsFilter *f);


extern
void FreeOutputBuffers(struct QsFilter *f);

extern
int stream_run_0p_0t(struct QsStream *s);


extern
void *makeRingBuffer(size_t *len, size_t *overhang);


extern
void freeRingBuffer(void *x, size_t len, size_t overhang);


