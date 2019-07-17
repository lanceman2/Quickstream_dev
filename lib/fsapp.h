/* This is a private header file that is only used in the faststream
 * libfsapp.so code. */


struct FsApp {

    // TODO: make this other than a simple linked list for
    // faster search.  Functions to change:
    // addFilterToList(), 
    //
    // List of filters.
    struct FsFilter *filters;
    struct Stream *streams;
};


struct FsStream {


};


struct FsFilter {

    int numOutputs;

    const char *name; // unique name for a given struct Fs

    int (* constructor)(void);
    int (* destructor)(void);
    int (* start)(int numInChannels, int numOutChannels);
    int (* stop)(int numInChannels, int numOutChannels);
    int (* input)(void *buffer, size_t len, int inputChannelNum);

    struct FsFilter *next; // next loaded filter

    struct FsFilter *outputs; // array of filters to output to this
};

