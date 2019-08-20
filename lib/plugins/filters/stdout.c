#include <stdio.h>

#include "../../../include/qsfilter.h"
#include "../../../lib/debug.h"

static int count;


int input(void *buffer, size_t len, uint32_t inputChannelNum) {

    DSPEW("Count=%d", count++);
    return 0; // success
}

