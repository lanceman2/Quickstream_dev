#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define QS_FILTER_NAME_CODE
#include "../../../../../include/qsfilter.h"
#include "../../../../../lib/debug.h"

#ifdef SPEW_LEVEL_DEBUG
static int count = 0;
#endif



void help(FILE *f) {
    fprintf(f,
        "test filter module that copies all input to a file\n"
        "\n"
        "                       OPTIONS\n"
        "\n"
        "      --maxWrite BYTES   default value %zu.  Maximum bytes\n"
        "                         written per input() call.\n"
        "\n"
        "      --file FILENAME    default value is stdout.\n"
        "\n"
        "\n",
        QS_DEFAULT_MAXWRITE);
}


static size_t maxWrite;
const char *filename = 0;
FILE *file = 0;


int construct(int argc, const char **argv) {

    DSPEW();

    maxWrite = qsOptsGetSizeT(argc, argv,
            "maxWrite", QS_DEFAULT_MAXWRITE);
  
    filename = qsOptsGetString(argc, argv, "file", 0);

    if(filename) {
        file = fopen(filename, "w+");
        if(!file) {
            ERROR("failed to open file \"%s\"", filename);
            return 1; // error
        }
    }
    else
        file = stdout;

    return 0; // success
}


int start(uint32_t numInPorts, uint32_t numOutPorts) {

    DSPEW("count=%d   %" PRIu32 " inputs and  %" PRIu32 " outputs",
            count++, numInPorts, numOutPorts);
    ASSERT(numInPorts == 1, "");
    ASSERT(numOutPorts == 0, "");

    return 0; // success
}


int input(const void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInPorts, uint32_t numOutPorts) {

    DASSERT(lens, "");
    DASSERT(numInPorts == 1, "");
    DASSERT(numOutPorts == 0, "");
    DASSERT(lens[0], "");

    int ret = 0;

    size_t wr = fwrite(buffers[0], 1, lens[0], file);

    if(wr != lens[0]) {
        ERROR("fwrite() failed to write %zu bytes", lens[0]);
        ret = 1; // error
    }

    qsAdvanceInput(0, wr);

    return ret;
}
