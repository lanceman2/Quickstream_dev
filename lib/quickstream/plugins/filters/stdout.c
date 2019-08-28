#include <stdio.h>

#include "../../../../include/qsfilter.h"
#include "../../../../lib/debug.h"

#ifdef SPEW_LEVEL_DEBUG
static int count = 0;
#endif


int input(void *buffer, size_t len, uint32_t inputChannelNum) {

    DSPEW("Count=%d", count++);
    return 0; // success
}

