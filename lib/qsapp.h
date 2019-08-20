/* This is a private header file that is only used in the quickstream
 * libqsapp.so code. */


struct QsApp {

    // This counter only increases.
    uint32_t filtersCount;

    // List of filters.  Singly linked list.
    struct QsFilter *filters;
    // List of streams.  Singly linked list.
    struct QsStream *streams;
};


struct QsStream {


};


struct QsFilter {

    void *dlhandle;

    uint32_t numOutputs;

    // idNum is gotten from QsApp::filtersCount at creation time.
    uint32_t idNum;

    char *name; // unique name for the Filter in a given app

    int (* construct)(void);
    int (* destroy)(void);
    int (* start)(int numInChannels, int numOutChannels);
    int (* stop)(int numInChannels, int numOutChannels);
    int (* input)(void *buffer, size_t len, int inputChannelNum);

    struct QsFilter *next; // next loaded filter

    struct QsFilter *outputs; // array of filters to output to this filter
};

