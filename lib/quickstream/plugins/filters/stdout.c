#include "../../../../include/qsfilter.h"
#include "../../../../lib/debug.h"

#ifdef SPEW_LEVEL_DEBUG
static int count = 0;
#endif

void help(FILE *f) {

}


int input(void *buffer, size_t len, uint32_t inputChannelNum,
        uint32_t flowState) {

    DSPEW("Count=%d", count++);

    return 0; // success
}

