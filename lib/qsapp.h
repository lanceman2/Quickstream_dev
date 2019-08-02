/* This is a private header file that is only used in the quickstream
 * libqsapp.so code. */


struct QsApp {

    // TODO: make this other than a simple linked list for
    // faster search.  Functions to change:
    // addFilterToList(), 
    //
    // List of filters.
    struct QsFilter *filters;
    struct QsStream *streams;
};


struct QsStream {


};


struct QsFilter {

    int numOutputs;

    const char *name; // unique name for a given struct Fs

    int (* constructor)(void);
    int (* destructor)(void);
    int (* start)(int numInChannels, int numOutChannels);
    int (* stop)(int numInChannels, int numOutChannels);
    int (* input)(void *buffer, size_t len, int inputChannelNum);

    struct QsFilter *next; // next loaded filter

    struct QsFilter *outputs; // array of filters to output to this
};

