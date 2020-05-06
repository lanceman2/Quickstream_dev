
// Controllers may setup pre and post filter input() callbacks.
// The filters keep a list of these:
//
struct Callback {

    int (*callback)(
            struct QsFilter *filter,
            const size_t len[],
            const bool isFlushing[],
            uint32_t numInputs, uint32_t numOutputs,
            void *userData);
    void *userData;
};
