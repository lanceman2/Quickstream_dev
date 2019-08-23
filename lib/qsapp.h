/* This is a private header file that is only used in the quickstream code
 * internal to libqsapp.so.  The libqsapp.so user, in general, should
 * not include this header file.  Hence this file is not installed. */

// This file is the guts of quickstream libqsapp API.  By understanding
// each data member in these data structures you can get a clear
// understanding of the insides of quickstream, and code it.  But it's
// likely that you will need to "scope the code" in app.c, filter.c,
// stream.c, thread.c, and process.c to get there.

struct QsApp {

    // List of filters.  Head of a singly linked list.
    struct QsFilter *filters;

    // List of streams.  Head of a singly linked list.
    struct QsStream *streams;

    // List of processes
    struct QsProcess *processes;
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

    // The array list of sources is created at start:
    uint32_t numSources;       // length of sources
    struct QsFilter **sources; // array of filter sources

    // filter connections:
    uint32_t numConnections;// length of from and to
    struct QsFilter **from; // array of filters
    struct QsFilter **to;   // array of filters

    struct QsStream *next; // next stream in app list
};


struct QsFilter {

    void *dlhandle; // from dlopen()

    struct QsApp *app;       // This does not change
    struct QsStream *stream; // This stream can be changed
    struct QsThread *thread; // thread that this filter will run in

 
    // The name never changes after filter loading/creation
    char *name; // unique name for the Filter in a given app

    // Callback functions that may be loaded.
    // Never change after loading/creation
    int (* construct)(void);
    int (* destroy)(void);
    int (* start)(uint32_t numInChannels, uint32_t numOutChannels);
    int (* stop)(uint32_t numInChannels, uint32_t numOutChannels);
    int (* input)(void *buffer, size_t len, uint32_t inputChannelNum);

    struct QsFilter *next; // next loaded filter in app list

    // numOutputs is calculated at start
    uint32_t numOutputs; // number of connected outputs
    struct QsFilter *outputs; // array of filters written to.
};

