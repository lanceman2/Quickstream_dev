#include "../../../../include/quickstream/filter.h"
#include "../../../../lib/debug.h"



void help(FILE *f) {

    fprintf(f,
"  Usage: nullSink\n"
"\n"
"  Reads N inputs.  This filter can have no outputs\n"
"  and does nothing with the input but consume it.\n"
"\n"
"  This may be essential for developing and testing filters.\n"
"\n"
    );
}


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInPorts, uint32_t numOutPorts) {

    DASSERT(lens);    // quickstream code error
    DASSERT(lens[0]); // quickstream code error

    ASSERT(numInPorts >= 1);  // user error
    ASSERT(numOutPorts == 0); // user error

    for(uint32_t i=0; i<numInPorts; ++i)
        if(lens[i])
            qsAdvanceInput(i, lens[0]);

    return 0; // success
}
