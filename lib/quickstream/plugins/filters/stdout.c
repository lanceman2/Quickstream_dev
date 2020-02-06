#include "../../../../include/quickstream/filter.h"
#include "../../../../lib/debug.h"



void help(FILE *f) {

    fprintf(f, "  Usage: stdout\n");
}


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInPorts, uint32_t numOutPorts) {

    DASSERT(lens);    // quickstream code error
    DASSERT(lens[0]); // quickstream code error

    ASSERT(numInPorts == 1);  // user error
    ASSERT(numOutPorts == 0); // user error


    size_t ret = fwrite(buffers[0], 1, lens[0], stdout);
    //fflush(stdout);

    qsAdvanceInput(0, ret);

    if(ret != lens[0]) {
        ERROR("fwrite(%p,1,%zu,stdout) failed",
            buffers[0], lens[0]);
        return -1; // ?
    }

    return 0; // success
}
