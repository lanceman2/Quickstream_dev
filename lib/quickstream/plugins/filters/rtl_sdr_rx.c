#include "../../../../include/quickstream/filter.h"
#include "../../../../lib/debug.h"



void help(FILE *f) {

    fprintf(f,

"    Usage: rtl_sdr_rx { --maxWrite LEN }\n"
"\n"
"  An I/Q receiver for the RTL2832 RTL-SDR USB dongle\n"
"\n"
"This filter is a source.\n"
"This filter must have 0 inputs.\n"
"This filter will read stdin and write it to 1 output.\n"
"\n"
"  The default value for LEN is %zu.\n"
"\n",
QS_DEFAULTMAXWRITE
        );
}


static size_t
    maxWrite,
    numSamples;

static int
    deviceIndex;

static uint32_t
    freq,
    sampRate;


int construct(int argc, const char **argv) {

    maxWrite = qsOptsGetSizeT(argc, argv, "maxWrite", QS_DEFAULTMAXWRITE);
    numSamples = qsOptsGetSizeT(argc, argv, "numSamples", 0);
    deviceIndex = qsOptsGetInt(argc, argv, "deviceIndex", 0);
    freq = qsOptsGetUint32(argc, argv, "freq", 100000000); // in Hz
    sampRate = qsOptsGetUint32(argc, argv, "sampRate", 2048000); // in Hz

    return 0; // success
}


int start(uint32_t numInPorts, uint32_t numOutPorts) {

    ASSERT(numInPorts == 0);
    ASSERT(numOutPorts == 1);

    qsCreateOutputBuffer(0, maxWrite);

    return 0; // success
}


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInputs, uint32_t numOutputs) {

    // We'll assume there is no input data.
    DASSERT(numInputs == 0);
    DASSERT(numOutputs == 1);

    // For output buffering from this filter.
    void *buffer = qsGetOutputBuffer(0, maxWrite, 0);

    // Put data in the output buffer.
    size_t rd = fread(buffer, 1, maxWrite, stdin);

    qsOutput(0, rd);

    // handle the stream closing and end of file.
    if(feof(stdin))
        // This filter is done reading stdin.
        return 1; // filter done.

    return 0; // continue.
}
