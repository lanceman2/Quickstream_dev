#include "../../../../include/qsfilter.h"
#include "../../../../lib/debug.h"

#if 0
static int count = 0;
#endif

void help(FILE *f) {

    // TODO: add a buffer size to flush option.
}

int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInPorts, uint32_t numOutPorts) {

    DASSERT(lens);
    DASSERT(lens[0]);
    ASSERT(numInPorts == 1);
    ASSERT(numOutPorts == 0);


    size_t ret = fwrite(buffers[0], 1, lens[0], stdout);
    fflush(stdout);

    qsAdvanceInput(0, ret);

    if(ret != lens[0]) return -1; // ?

    return 0; // success
}
