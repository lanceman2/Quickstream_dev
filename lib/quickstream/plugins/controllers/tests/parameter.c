// This controller module is part of a test in
// tests/372_controller_parameter
//
// If you change this file you'll likely brake that test.  If you need to
// edit this, make sure that that you also fix
// tests/372_controller_parameter, without changing the nature of that
// test.


#include <stdio.h>

#include "../../../../../include/quickstream/app.h"
#include "../../../../../include/quickstream/parameter.h"
#include "../../../../../include/quickstream/controller.h"
#include "../../../../../lib/debug.h"



void help(FILE *f) {
    fprintf(f,
"   Usage: tests/dummy\n"
"\n"
" A test controller module that makes and tests Parameters.\n"
"\n"
"\n");
}

static
uint64_t count = 0;


static
int setCount(void *value, const char *pName,
            void *userData) {

    

    return 0;
}



int construct(int argc, const char **argv) {

    DSPEW("in construct()");

    qsParameterCreate("count", QsUint64, setCount, 0);

    printf("%s()\n", __func__);

    return 1; // success
}

int preStart(struct QsStream *stream) {

    DSPEW();
    count = 0;
    printf("%s()\n", __func__);
    return 0;
}

int postStart(struct QsStream *stream) {

    DSPEW();
    printf("%s()\n", __func__);
    return 0;
}

int preStop(struct QsStream *stream) {

    DSPEW();
    printf("%s()\n", __func__);
    return 0;
}

int postStop(struct QsStream *stream) {

    DSPEW();
    printf("%s()\n", __func__);
    return 0;
}

int destroy(void) {

    DSPEW();
    printf("%s()\n", __func__);
    return 0; // success
}
