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


int input(const void *buffer[], const size_t len[],
        const bool isFlushing[],
        uint32_t numInputs, uint32_t numOutputs) {

    // We'll assume there is no input data.
    DASSERT(len == 0, "");

    // For output buffering.
    buffer = qsGetBuffer(0);
    len = 1;

    // TODO: handle the stream closing.

    enum QsFilterInputReturn ret = QsFContinue;

    size_t rd = fread(buffer, 1, len, stdin);

    if(feof(stdin)) { ret = QsFFinished; }

    // Output to all output channels
    if(rd)
        qsOutput(rd, 0);

    return ret; // success
}
