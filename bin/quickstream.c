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
#include <stdbool.h>

#include "../include/qsapp.h"

// Turn on spew macros for this file.
#ifndef SPEW_LEVEL_DEBUG
#  define SPEW_LEVEL_DEBUG
#endif
#include "../lib/debug.h"
#include "getOpt.h"



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
        "    What filter modules to run with are given in command-line options.\n"
        "  This program takes action after each command-line argument it parses,\n"
        "  so the order of command-line arguments is very important.  A connect\n"
        "  option, --connect, before you load any filters will have no effect.\n"
        "\n"
        "    All command line options require an preceding option flag.  All\n"
        "  command line options with no arguments may be given in any of two\n"
        "  forms.  The two argument option forms below are equivalent:\n"
        "\n"
        "     -d\n"
        "     --display\n"
        "\n"
        "  -display is not a valid option.\n"
        "\n"
        "  All command line options with arguments may be given in any of three\n"
        "  forms.  The three option forms below are equivalent:\n"
        "\n"
        "     -f stdin\n"
        "     --filter stdin\n"
        "     --filter=stdin\n"
        "\n"
        "     -filter stdin\n"
        "   and\n"
        "     -filter=stding\n"
        "   or not valid options.\n"
        "\n"
        "----------------------------------------------------------------------\n"
        "                            OPTIONS\n"
        "----------------------------------------------------------------------\n"
        "\n"
        "  -c|--connect [SEQUENCE]   connect loaded filters in a stream.\n"
        "                            Loaded filters are numbered starting at\n"
        "               zero.  For example:\n"
        "\n"
        "                     --connect \"0 1 2 4\"\n"
        "\n"
        "               will connect the from filter 0 to filter 1 and from\n"
        "               filter 2 to filter 4, where filter 0 is the first\n"
        "               loaded filter, filter 1 is the second loaded filter,\n"
        "               and so on.  If no SEQUENCE argument is given, all\n"
        "               filters that are not connected yet and that are\n"
        "               currently loaded will be connected in the order that\n"
        "               they have been loaded.\n"
        "\n"
        "\n"
        "  -d|--display  display a dot graph of the stream before running it.\n"
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



int main(int argc, const char * const *argv) {

    signal(SIGSEGV, gdb_catcher);

    // This program will only construct one QsApp and one QsStream.

    struct QsApp *app = 0;
    struct QsStream *stream = 0;
    int numFilters = 0;
    struct QsFilter **filters = 0;
    bool gotConnection = false;
    bool verbose = false;

    int i = 1;
    const char *arg = 0;

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

        int c = getOpt(argc, argv, &i, options, &arg);

        switch(c) {

            case -1:
            case '*':
                fprintf(stderr, "Bad option: %s\n\n", arg);
            case 'h':
            case '?':
                return usage(argv[0]);

            case 'V':
                printf("%s\n", QS_VERSION);
                return 0;
            case 'v':
                verbose = true;
                break;

            case 'c':
                if(numFilters < 2) {
                    fprintf(stderr, "Got --connect "
                            "option with %d filter(s) loaded\n",
                            numFilters);
                    return usage(argv[0]);
                }

                // Example:
                //
                //   --connection "0 1 1 2"
                //
                //           filter connections ==> (0) -> (1) -> (2)
                //
        
                if(!arg || arg[0] < '0' || arg[0] > '9') {
                    // There is no connection list, so by default we
                    // connect filters in the order they are loaded.
                    for(int i=1; i<numFilters; ++i) {
                        fprintf(stderr,"connecting: %d -> %d\n", i-1, i);
                        qsStreamConnectFilters(stream, filters[i-1], filters[i]);
                    }
                    break;
                }
                char *endptr = 0;
                for(const char *str = arg; *str && endptr != str;) {
                    long from = strtol(str, &endptr, 10);
                    if(endptr == str) {
                        fprintf(stderr, "Bad --connect \"%s\" values "
                                "out of range\n\n", arg);
                        return usage(argv[0]);
                    }
                    str = endptr;
                    long to = strtol(str, &endptr, 10);
                    if(endptr == str) {
                        fprintf(stderr, "Bad --connect \"%s\" values "
                                "out of range\n\n", arg);
                        return usage(argv[0]);
                    }
                    str = endptr;
                    // Check values read.
                    if(from < 0 || to < 0 || from >= numFilters
                            || to >= numFilters) {
                        fprintf(stderr, "Bad --connect \"%s\" values "
                                "out of range\n\n", arg);
                        return usage(argv[0]);
                    }

                    fprintf(stderr, "connecting: %ld -> %ld\n", from, to);

                    if(qsStreamConnectFilters(stream, filters[from],
                                filters[to])) {
                        return 1; // failed
                    }
                    gotConnection = true;
                }

                DSPEW("option %c = %s", c, arg);

                // next
                arg = 0;
                ++i;

                break;

            case 'd':
                // display a dot graph
                if(!app) break; // nothing to display yet.
                qsAppDisplayFlowImage(app, QSPrintDebug, true/*waitForDisplay*/);
                break;
            case 'f': // Load filter module
                if(!arg) {
                    fprintf(stderr, "Bad --filter option\n\n");
                    return usage(argv[0]);
                }
                filters = realloc(filters, sizeof(*filters)*(++numFilters));
                ASSERT(filters, "realloc(,%zu) failed",
                        sizeof(*filters)*numFilters);
                if(!app) {
                    app = qsAppCreate();
                    ASSERT(app, "");
                    stream = qsAppStreamCreate(app);
                    ASSERT(stream, "");
                }
                const char *name = 0;

                // example:
                //  --filter stdin [ --name stdinput --bufferSize 5012 ]
                //
                int fargc = 0; // 4
                const char * const *fargv = 0; // points to --name

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
                if(verbose) {
                    fprintf(stderr, "Got filter args[%d]= [", fargc);
                    for(int j=0; j<fargc; ++j)
                        fprintf(stderr, "%s ", fargv[j]);
                    fprintf(stderr, "]\n");
                }
                
                filters[numFilters-1] = qsAppFilterLoad(app,
                        arg, name, fargc, (const char **) fargv);
                if(!filters[numFilters-1]) return 1; // error

                // next
                arg = 0;

                break;

            default:
                // This should not happen.
                fprintf(stderr, "unknown option character: %d\n", c);
                return usage(argv[0]);
        }
    }

    DASSERT(numFilters, "");

    DSPEW("Done parsing command-line arguments");

    if(!gotConnection) {
        // There is no connection list, so by default we
        // connect filters in the order they are loaded.
        for(int i=1; i<numFilters; ++i) {
            qsStreamConnectFilters(stream, filters[i-1], filters[i]);
        }
    }


    WARN("SUCCESS");

    return 0;
}
