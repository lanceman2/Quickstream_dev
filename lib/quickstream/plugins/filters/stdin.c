#include "../../../../include/qsfilter.h"
#include "../../../../lib/debug.h"

void help(FILE *f) {

    fprintf(f,
        "This filter is usually a source.\n"
        "This filter will ignore all inputs.\n"
        "This filter will read stdin and write it to all outputs.\n"
        "\n"
// TODO:
        "TODO: add a stdin fread() buffer size option.  Without it when there\n"
        "is relatively slow input data this may block on the fread() call to\n"
        "to make the stream function properly.\n"
        "\n");
}


static const size_t OutLen = 1024;


int input(const void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInputs, uint32_t numOutputs) {

    // We'll assume there is no input data.
    DASSERT(numInputs == 0, "");

    // For output buffering.
    void *buffer = qsGetBuffer(0, OutLen);

    // TODO: handle the stream closing.

    enum QsFilterInputReturn ret = QsFContinue;

    // Put data in the output buffer.
    size_t rd = fread(buffer, 1, OutLen, stdin);

    if(feof(stdin)) { ret = QsFFinished; }

    // Output to all output channels
    if(rd) {
        size_t outLens[numOutputs];
        for(uint32_t i=0; i<numOutputs; ++i)
            outLens[i] = rd;
        qsOutputs(outLens);
    }

    return ret;
}
