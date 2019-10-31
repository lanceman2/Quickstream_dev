#include "../../../../include/qsfilter.h"
#include "../../../../lib/debug.h"

#if 0
static int count = 0;
#endif

void help(FILE *f) {

    // TODO: add a buffer size to flush option.
}


int input(void *buffer, size_t len, uint32_t inputChannelNum,
        uint32_t flowState) {

    DASSERT(len, "");
    fwrite(buffer, 1, len, stdout);
    fflush(stdout);

    return 0; // success
}
