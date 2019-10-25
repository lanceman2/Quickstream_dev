#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h> 

#include "./qs.h"
#include "./debug.h"


struct QsOpts {

    int argc;
    const char **argv;
};


// NOTE: argv[][] may or may not be null terminated.


void qsOptsInit(struct QsOpts *opts, int argc, const char **argv) {

    DASSERT(opts, "");

    opts->argc = argc;
    opts->argv = argv;
}

// Checks for: "--option"
static inline
bool FindOptPresent(struct QsOpts *opts, const char *opt) {

    size_t optLen = strlen(opt);

    for(int i=opts->argc-1; i>=0; --i) {
        const char *arg = opts->argv[i];
        size_t argLen = strlen(arg);
        if(argLen == optLen+2 && // --option
                arg[0] == '-' && arg[1] == '-' &&
                strncmp(opt, &arg[2], optLen) == 0)
            return true;
    }

    return false;
}


// checks for:  --option val
//         or:  --option=val
//
//  returns: val or 0
static inline
const char *FindOptString(struct QsOpts *opts, const char *opt) {

    size_t optLen = strlen(opt);

    for(int i=opts->argc-1; i>=0; --i) {
        const char *arg = opts->argv[i];
        size_t argLen = strlen(arg);
        if(argLen >= optLen + 2 && // --option
                arg[0] == '-' && arg[1] == '-' &&
                strncmp(opt, &arg[2], optLen) == 0) {
            // got:  --option or more
            if(argLen > optLen+2 && arg[optLen+2] == '=')
                // got: --option=val
                return &arg[optLen+3];
            else if(argLen == optLen+2 && i < opts->argc - 1)
                // got: --option val
                return opts->argv[i + 1];
            // else got: --option
            // with no more args.
            // This is kind-of an error case.
            return 0;
        }
    }

    return 0;
}


float qsOptsGetFloat(struct QsOpts *opts, const char *optName,
        float defaultVal) {

    const char *str = FindOptString(opts, optName);
    if(str) return strtof(str, 0);
    return defaultVal;
}


double qsOptsGetDouble(struct QsOpts *opts, const char *optName,
        double defaultVal) {

    const char *str = FindOptString(opts, optName);
    if(str) return strtod(str, 0);
    return defaultVal;
}


const char *qsOptsGetString(struct QsOpts *opts, const char *optName,
        const char *defaultVal) {

    const char *str = FindOptString(opts, optName);
    if(str) return str;
    return defaultVal;
}


int qsOptsGetInt(struct QsOpts *opts, const char *optName,
        int defaultVal) {

    const char *str = FindOptString(opts, optName);
    if(str) return (int) strtol(str, 0, 10);
    return defaultVal;
}


int qsOptsGetUint32(struct QsOpts *opts, const char *optName,
        uint32_t defaultVal) {

    const char *str = FindOptString(opts, optName);
    if(str) return (uint32_t) strtoul(str, 0, 10);
    return defaultVal;
}
