#include <stdio.h>

#include "../../../../../include/quickstream/app.h"
#include "../../../../../include/quickstream/controller.h"
#include "../../../../../lib/debug.h"



void help(FILE *f) {
    fprintf(f,
"  Usage: tests/dummy\n"
"\n"
"A test controller module that does nothing.\n"
"\n"
"\n");
}


int construct(int argc, const char **argv) {

    DSPEW("in construct()");

    return 0; // success
}
