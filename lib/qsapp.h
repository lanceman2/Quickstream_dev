/* This is a private header file that is only used in the quickstream
 * libqsapp.so code. */


struct QsApp {

    // List of filters.  Singly linked list.
    struct QsFilter *filters;

    // List of streams.  Singly linked list.
    struct QsStream *streams;

    // List of processes
    struct QsProcess *processes;
};


struct QsThread {

    uint32_t *filters; // array of filter IDs
};


struct QsProcess {

    struct QsThread *threads; // array of threads
};


struct QsStream {

    struct QsApp *app;

    struct QsFilter *sources; // array of filter sources

    // filter connections:
    uint32_t numConnections;
    struct QsFilter **from; // array of filters
    struct QsFilter **to;   // array of filters

    struct QsStream *next; // next stream in app list
};


struct QsFilter {

    void *dlhandle;

    struct QsApp *app;
    struct QsStream *stream;

    uint32_t numOutputs;

    char *name; // unique name for the Filter in a given app

    int (* construct)(void);
    int (* destroy)(void);
    int (* start)(int numInChannels, int numOutChannels);
    int (* stop)(int numInChannels, int numOutChannels);
    int (* input)(void *buffer, size_t len, int inputChannelNum);

    struct QsFilter *next; // next loaded filter in app list

    struct QsFilter *outputs; // array of filters to output to this filter
};

