/** \page quickstream_program The stream runner program
 
\tableofcontents
quickstream is a command-line program that can load, connect filters, and
run the loaded filters in a flow stream
\section quickstream_args command-line arguements
\section quickstream_examples
\subsection quickstream_helloworld
*/



#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "../include/qsapp.h"

// Turn on spew macros for this file.
#ifndef SPEW_LEVEL_DEBUG
#  define SPEW_LEVEL_DEBUG
#endif
#include "../lib/debug.h"



static void gdb_catcher(int signum) {

    fprintf(stderr, "\n\n  Caught signal %d\n", signum);
    fprintf(stderr, "\n\n  try running:\n\n   gdb -pid %u\n\n", getpid());
    while(1) usleep(10000);
}


static
int usage(const char *argv0) {

    fprintf(stderr,
        "  Usage: %s OPTIONS\n"
        "\n"
        "    Run a quickstream flow graph.\n"
        "\n"
        "  What filter modules to run with are given in command-line options.\n"
        "  This program takes action after each command-line argument it parsed,\n"
        "  so the order of command-line arguments is very important.  You cannot\n"
        "  call --connect before you load any filters.\n"
        "\n"
        "------------------------------------------------------------------\n"
        "                            OPTIONS\n"
        "------------------------------------------------------------------\n"
        "\n"
        "  -c|--connect SEQUENCE   connect loaded filters in a stream.\n"
        "                          Loaded filters are numbered starting at\n"
        "               zero.  For example:\n"
        "\n"
        "                     --connect \"0 1 2 4\"\n"
        "\n"
        "               will connect the from filter 0 to filter 1 and from\n"
        "               filter 2 to filter 4, where filter 0 is the first\n"
        "               loaded filter, filter 1 is the second loaded filter,\n"
        "               and so on.\n"
        "\n"
        "\n"
        "  -f|--filter FILENAME [ args .. ]  load filter module with filename\n"
        "                                    FILENAME.  An independent instance\n"
        "               of the filter will be created for each time a filter is\n"
        "               loaded and the filters will not share virtual addresses\n"
        "               and variables.  Optional arguments passed in square\n"
        "               brackets (with spaces around the brackets) are passed to\n"
        "               the module construct() function.\n"
        "\n"
        "\n"
        "   -h|--help   print this help to stderr and exit.\n"
        "\n"
        "\n"
        "   -V|--version  print %s version information to stdout an than exit.\n"
        "\n",
        argv0, argv0);

    return 1; // non-zero error code
}



int main(int argc, char **argv) {

    signal(SIGSEGV, gdb_catcher);

    // This program will only construct one QsApp and one QsStream.

    struct QsApp *app = 0;
    struct QsStream *stream = 0;
    int numFilters = 0;
    struct QsFilter **filters = 0;

    DSPEW("app=%p stream=%p filters=%p", app, stream, filters);
    optind = 1;

    while(1) {

        int i = 0;
        optarg = 0;
        static const struct option long_options[] = {
            {"connect", false/*require arg*/,0,  'c'  },
            {"filter",  true/*require arg*/, 0,  'f'  },
            {"help",    false,               0,  'h'  },
            {"verbose", false,               0,  'v'  },
            {"version", false,               0,  'V'  },
            {0,         0,                   0,  0    }
        };

        int c = getopt_long_only(argc, argv, "c:f:hvV",
                    long_options, &i);

        if(c == -1)
            break;

        switch(c) {

            case 'h':
                return usage(argv[0]);

            case 'V':
                printf("%s\n", QS_VERSION);
                return 0;

            case 'c':
                if(!filters) return usage(argv[0]);
                DSPEW("option %c|%s = %s", c, long_options[i].name, optarg);
                break;

            case 'f': // Load filter module
                if(!optarg) return usage(argv[0]);
                filters = realloc(filters, sizeof(*filters)*(++numFilters));
                ASSERT(filters, "realloc(,%zu) failed",
                        sizeof(*filters)*numFilters);
                if(!app) {
                    app = qsAppCreate();
                    ASSERT(app, "");
                    stream = qsAppStreamCreate(app);
                    ASSERT(stream, "");
                }
                char *name = 0;
                int fargc = 0;
                char **fargv = 0;
                DSPEW(" optind=%d", optind);
                if(optind < argc && strcmp(argv[optind], "[") == 0) {
                    fargv = &argv[++optind];
                    while(optind < argc && strcmp(argv[optind], "]")) {
                        if(strcmp(argv[optind],"--name") == 0) {
                            ++optind;
                            ++fargc;
                            if(optind < argc && strcmp(argv[optind], "]")) {
                                name = argv[optind];
                                ++optind;
                                ++fargc;
                            }
                        } else {
                            ++optind;
                            ++fargc;
                        }
                    }
                    if(strcmp(argv[optind], "]") == 0) ++optind;
                }
                printf("Got filter args[%d]= [", fargc);
                for(int j=0; j<fargc; ++j)
                    printf("%s ", fargv[j]);
                printf("]\n");
                filters[numFilters-1] = qsAppFilterLoad(app,
                        optarg, name, fargc, (const char **) fargv);
                if(!filters[numFilters-1]) return 1; // error
                break;

            case '?':
                fprintf(stderr, "\n");
                return usage(argv[0]);
        }

        if(optind > argc) break;
    }

    DASSERT(numFilters, "");

    DSPEW("Done parsing command-line arguments");




    WARN("SUCCESS");

    return 0;
}
