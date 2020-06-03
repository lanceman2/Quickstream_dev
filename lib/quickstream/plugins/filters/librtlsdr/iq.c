#include <rtl-sdr.h>

#include "../../../../../include/quickstream/filter.h"
#include "../../../../../lib/debug.h"



void help(FILE *f) {

    fprintf(f,

"  Usage: iq\n"
"\n"
"This filter is a source.\n"
"This filter must have 0 inputs.\n"
"\n"
"\n"
"                 OPTIONS\n"
"\n"
"\n"
"\n"
        );
}


static rtlsdr_dev_t *dev = 0;

int construct(int argc, const char **argv) {




    return 0; // success
}


int start(uint32_t numInPorts, uint32_t numOutPorts) {

    ASSERT(numInPorts == 0);
    ASSERT(numOutPorts == 1);

    int r = rtlsdr_open(&dev, 0);

    WARN("rtlsdr_open()=%d", r);

    return 0; // success
}


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInputs, uint32_t numOutputs) {


    return 0; // continue.
}
