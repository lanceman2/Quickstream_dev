#include "../../../../include/qsfilter.h"
#include "../../../../lib/debug.h"

void help(FILE *f) {

    fprintf(f,
        "This filter is usually a source.\n"
        "This filter will ignore all inputs.\n"
        "This filter will read stdin and write it to all outputs.\n");
}


int input(void *buffer, size_t len, uint32_t inputChannelNum,
        uint32_t flowState) {

    // For output buffering.
    buffer = qsGetBuffer(0);
    len = QS_DEFAULTWRITELENGTH;

    // TODO: handle the stream closing.

    enum QsFilterInputReturn ret = QsFContinue;

    if(len != fread(buffer, 1, len, stdin)) {

        if(feof(stdin)) { ret = QsFFinished; }

        ERROR("fread(,,,stdin) failed");
        return -1; // error
    }

    // Output to all output channels
    //
    // This is the default after return:
    qsOutput(len, 0);

    return ret; // success
}
