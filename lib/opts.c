#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#include "./qs.h"
#include "./debug.h"


struct QsOpts {



};



void qsOptsInit(struct QsOpts *opts, int argc, const char **argv) {

}

float qsOptsGetFloat(struct QsOpts *opts, const char *optName,
        float defaultVal) {

    return defaultVal;
}

double qsOptsGetDouble(struct QsOpts *opts, const char *optName,
        double defaultVal) {

    return defaultVal;
}

const char *qsOptsGetString(struct QsOpts *opts, const char *optName,
        const char *defaultVal) {

    return defaultVal;
}

int qsOptsGetInt(struct QsOpts *opts, const char *optName,
        int defaultVal) {

    return defaultVal;
}
