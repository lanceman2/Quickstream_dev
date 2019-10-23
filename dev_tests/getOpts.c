/** \page quickstream_program The stream runner program
 
\tableofcontents
quickstream is a command-line program that can load, connect filters, and
run the loaded filters in a flow stream
\section quickstream_args command-line arguements
\section quickstream_examples
\subsection quickstream_helloworld
*/



#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

// Turn on spew macros for this file.
#include "../bin/getOpt.h"
#include "../lib/debug.h"



static void gdb_catcher(int signum) {

    fprintf(stderr, "\n\n  Caught signal %d\n", signum);
    fprintf(stderr, "\n\n  try running:\n\n   gdb -pid %u\n\n", getpid());
    while(1) usleep(10000);
}


static
int usage(const char *argv0) {

    fprintf(stderr,
        "  Usage: %s OPTIONS\n\n" , argv0);

    return 1; // non-zero error code
}

#define VERSION "0.1.42"


int main(int argc, const char * const *argv) {

    signal(SIGSEGV, gdb_catcher);

    // This program will only construct one QsApp and one QsStream.

    int i = 1;
    const char *arg = 0;
    int numFilters = 0;
    while(i < argc) {

        const struct opts options[] = {
            { "connect", 'c' },
            { "display", 'd' },
            { "filter", 'f' },
            { "help", 'h' },
            { "help", '?' },
            { "version", 'V' },
            { "verbose", 'v' },
            { 0, 0 }
        };

        fprintf(stderr, "getOpt( argv[%d]=%s\n", i, argv[i]);
        int c = getOpt(argc, argv, &i, options, &arg);

        switch(c) {

            case -1:
            case '*':
                fprintf(stderr, "Bad option: %s\n\n", arg);
            case 'h':
            case '?':
                fprintf(stderr, "Got option: %c\n\n", c);
                return usage(argv[0]);

            case 'V':
                printf("%s\n", VERSION);
                return 0;

            case 'c':

                if(numFilters < 2) {
                    fprintf(stderr, "Not 2 filters loaded yet\n");
                
                    return usage(argv[0]);
                }

                // Example:
                //
                //   --connection "0 1 1 2"
                //
                //           filter connections ==> (0) -> (1) -> (2)
                //
        
                if(!arg || arg[0] == '-') {
                    // There is no connection list, so by default we
                    // connect filters in the order they are loaded.
                    for(int i=1; i<numFilters; ++i)
                        fprintf(stderr, " ---        --- connecting: %d -> %d\n", i-1, i);
                    break;
                }
                char *endptr = 0;
                for(const char *str = arg; *str && endptr != str;) {
                    long from = strtol(str, &endptr, 10);
                    if(endptr == str) {
                        fprintf(stderr, "Bad --connect args: \"%s\" value "
                                "out of range\n\n", str);
                        return usage(argv[0]);
                    }
                    fprintf(stderr, "str=\"%s\"\n", str);
                    str = endptr;
                    long to = strtol(str, &endptr, 10);
                    if(endptr == str) {
                        fprintf(stderr, "Bad --connect args: \"%s\" value "
                                "out of range\n\n", str);
                        return usage(argv[0]);
                    }
                    str = endptr;
                    // Check values read.
                    if(from < 0 || to < 0 || from >= numFilters || to >= numFilters) {
                        fprintf(stderr, "Bad --connect args: \"%s\" value "
                                "out of range\n\n", arg);
                        return usage(argv[0]);
                    }

                    fprintf(stderr, "  ** connecting: %ld -> %ld\n", from, to);
                }

                DSPEW("option %c = %s", c, arg);

                // next
                arg = 0;
                ++i;

                break;

            case 'd':

                fprintf(stderr, "Got option: --display\n");
                break;
            case 'f': // Load filter module
                if(!arg) {
                    fprintf(stderr, "Bad --filter option\n\n");
                    return usage(argv[0]);
                }
                const char *name = 0;
                int fargc = 0;
                const char * const *fargv = 0;
                if(i+1 < argc && strcmp(argv[i+1], "[") == 0) {
                    ++i;
                    fargv = &argv[++i];
                    while(i < argc && strcmp(argv[i], "]")) {
                        if(strcmp(argv[i],"--name") == 0) {
                            ++i;
                            ++fargc;
                            if(i < argc && strcmp(argv[i], "]")) {
                                name = argv[i];
                                ++i;
                                ++fargc;
                            }
                        } else {
                            ++i;
                            ++fargc;
                        }
                    }
                    if(strcmp(argv[i], "]") == 0) ++i;
                } else
                    ++i;
                fprintf(stderr, "Got filter args[%d]= [", fargc);
                for(int j=0; j<fargc; ++j)
                    fprintf(stderr, "%s ", fargv[j]);
                fprintf(stderr, "]\n");
                fprintf(stderr, "Loading filter: %s  name=%s\n", arg, name);

                // next
                arg = 0;

                ++numFilters;

    fprintf(stderr, "Next opt is: argv[%d]=%s\n", i, argv[i]);

                break;

            default:
                // This should not happen.
                fprintf(stderr, "unknown option character: %d\n", c);
                return usage(argv[0]);
        }
    }

    DSPEW("Done parsing command-line arguments");

    WARN("SUCCESS");

    return 0;
}
