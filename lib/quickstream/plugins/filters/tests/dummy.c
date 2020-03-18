#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "../../../../../include/quickstream/filter.h"
#include "../../../../../lib/debug.h"



void help(FILE *f) {
    fprintf(f,
"  Usage: tests/dummy\n"
"\n"
"A test filter module that does nothing.\n"
"\n"
"\n");
}


int input(void *buffers[], const size_t lens[],
        const bool isFlushing[],
        uint32_t numInPorts, uint32_t numOutPorts) {

    return 0; // success
}
