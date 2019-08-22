/* This is a private header file that is only used in the quickstream code
 * internal to libqsapp.so.  The libqsapp.so user, in general, should
 * not include this header file.  Hence this file is not installed. */


struct QsApp {

    // List of filters.  Singly linked list.
    struct QsFilter *filters;

    // List of streams.  Singly linked list.
    struct QsStream *streams;

    // List of processes
    struct QsProcess *processes;
};


struct QsThread {

    struct QsStream *stream; // stream that owns this thread
    struct QSProcess *process;
};


struct QsProcess {

    struct QsStream *stream; // stream that owns this process

    uint32_t numThreads;
    struct QsThread *threads; // array of threads
};


struct QsStream {

    struct QsApp *app;

    uint32_t numSources;
    struct QsFilter **sources; // array of filter sources

    // filter connections:
    uint32_t numConnections;
    struct QsFilter **from; // array of filters
    struct QsFilter **to;   // array of filters

    struct QsStream *next; // next stream in app list
};


struct QsFilter {

    void *dlhandle;

    struct QsApp *app; // This does not change
    struct QsStream *stream; // This stream can be changed
    struct QsThread *thread; // thread that this filter will run in

    uint32_t numOutputs;

    char *name; // unique name for the Filter in a given app

    // Callback functions that may be loaded.
    int (* construct)(void);
    int (* destroy)(void);
    int (* start)(int numInChannels, int numOutChannels);
    int (* stop)(int numInChannels, int numOutChannels);
    int (* input)(void *buffer, size_t len, int inputChannelNum);

    struct QsFilter *next; // next loaded filter in app list

    struct QsFilter *outputs; // array of filters that this filter writes to.
};

