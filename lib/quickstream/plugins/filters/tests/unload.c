#include "../../../../../include/quickstream/filter.h"
#include "../../../../../lib/debug.h"


void help(FILE *f) {

    fprintf(f,
        "  USAGE: unload [ --return VAL ]\n"
        "\n"
        "This filter is just a test that unloads itself in construct().\n"
        "\n");
}



int construct(int argc, const char **argv) {

    DSPEW();

    int ret = qsOptsGetInt(argc, argv, "return", 1);

    // We don not let it return 0.
    if(ret) return ret;

    return 1;
}


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInputs, uint32_t numOutputs) {

    return 0;
}
