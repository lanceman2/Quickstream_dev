#include "../../../../include/qsfilter.h"
#include "../../../../lib/debug.h"

#if 0
static int count = 0;
#endif

void help(FILE *f) {

    // TODO: add a buffer size to flush option.
}

int input(const void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInPorts, uint32_t numOutPorts) {

    DASSERT(lens, "");
    DASSERT(lens[0], "");
    DASSERT(numInPorts == 1, "");
    DASSERT(numOutPorts == 0, "");


    fwrite(buffers[0], 1, lens[0], stdout);
    fflush(stdout);

    qsAdvanceInput(0, lens[0]);

    return 0; // success
}
