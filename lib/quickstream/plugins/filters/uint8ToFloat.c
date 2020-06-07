#include "../../../../include/quickstream/filter.h"
#include "../../../../lib/debug.h"



void help(FILE *f) {

    fprintf(f,

"    Usage: uint8ToFloat { --maxWrite LEN }\n"
"\n"
"  This has one input and one output.\n"
"\n"
"  It assumes that the input is unsigned bytes (uint8_t) and converts each\n"
"  byte to a float in a new output buffer.\n"
"\n"
"                 OPTIONS\n"
"\n"
"\n"
"  --maxWrite LEN  Set the maximum write promise to LEN bytes.  The\n"
"                  default value for LEN is %zu.  The LEN will get\n"
"                  rounded up to a multiple of sizeof(float).\n"
"\n"
"\n",
QS_DEFAULTMAXWRITE
        );
}


static size_t maxWrite;


int construct(int argc, const char **argv) {

    maxWrite = qsOptsGetSizeT(argc, argv, "maxWrite", QS_DEFAULTMAXWRITE);

    if(maxWrite % sizeof(float))
        // Make maxWrite closest multiple of sizeof(float) by adding.
        maxWrite += sizeof(float) - maxWrite % sizeof(float);

    return 0; // success
}


int start(uint32_t numInPorts, uint32_t numOutPorts) {

    ASSERT(numInPorts == 1);
    ASSERT(numOutPorts == 1);

    qsCreateOutputBuffer(0, maxWrite);

    return 0; // success
}


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInputs, uint32_t numOutputs) {

    size_t numBytesIn = lens[0];
    size_t outLen = numBytesIn*(sizeof(float));

    if(outLen > maxWrite)
        outLen = maxWrite;

    numBytesIn = outLen/sizeof(float);

    // Now the ratio of rdLen to outLen is 1 to 4.
    // And we will not overflow the output buffer.

    float *outBuf = qsGetOutputBuffer(0, maxWrite, 0);
    float *outEnd = outBuf + numBytesIn;
    uint8_t *in = buffers[0];
    while(outBuf < outEnd)
        // Copy and convert from byte to float.
        *(outBuf++) = *(in++);

    qsAdvanceInput(0/*port*/, numBytesIn);
    qsOutput(0/*port*/, outLen);

    return 0; // continue.
}
