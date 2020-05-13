
// Controllers may setup post filter input() callbacks.
// The filters keep a list of these:
//
struct ControllerCallback {

    int (*callback)(
            struct QsFilter *filter,
            const size_t lenIn[],
            const size_t lenOut[],
            const bool isFlushing[],
            uint32_t numInputs, uint32_t numOutputs,
            void *userData);
    void *userData;

    // This is used to mark that a non-zero value was returned from the
    // callback() call.  And the callback is stopped being called if
    // non-zero was returned.
    int returnValue;

    // Used make a singly linked list of callbacks to remove.
    //
    // Needed because the QsDictionary has no way to iterate through it
    // while it is edited.
    //
    struct ControllerCallback *next;

    // We need the stink'n key in order to remove the callback.  It hurts
    // so much to have to add another 64 bit pointer.
    const char *key;
};
