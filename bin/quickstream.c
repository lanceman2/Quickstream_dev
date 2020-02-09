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

#include "../include/quickstream/app.h"

// Turn on spew macros for this file.
#ifndef SPEW_LEVEL_DEBUG
#  define SPEW_LEVEL_DEBUG
#endif
#include "../lib/debug.h"
#include "getOpt.h"



static void gdb_catcher(int signum) {

    ASSERT(0, "Caught signal %d\n", signum);
}

#define DEFAULT_MAXTHREADS  ((uint32_t) 5)


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
"    This program executes code after parsing each command line option\n"
"  in the order that the options are given.  After code for each command\n"
"  line option is executed the program will terminate.\n"
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
"  -d|--display  display a dot graph of the stream.  If display is\n"
"                called after the stream is readied (via --ready) this\n"
"               will show stream channel and buffering details.\n"
"\n"
"\n"
"  -D|--display-wait  like --display but this waits for the display\n"
"                     program to exit before going on to the next\n"
"               argument option.\n"
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
"  -p|--plug \"FROM_F TO_F FROM_PORT TO_PORT\"  connects two filters with\n"
"                                               more control over which\n"
"               ports get connected.  FROM_F refers to the filter we\n"
"               are connecting from as a number in order in which the\n"
"               filters are loading starting at 0.  TO_F refers to the\n"
"               filter we are connecting to as a number in order in\n"
"               which the filters are loading starting at 0.  FROM_PORT\n"
"               refers to the port number that we are connecting from\n"
"               as viewed from the \"from filter\".  TO_PORT refers to\n"
"               the port number that we are connecting to as viewed\n"
"               from the \"to filter\".\n"
"\n"
"\n" 
"   -R|--ready  ready the stream.  This calls all the filter start()\n"
"               functions that exist and gets the stream ready to flow,\n"
"               except for spawning worker flow threads.\n"
"\n"
"\n" 
"   -r|--run    run the stream.  This readies the stream and runs it.\n"
"\n"
"\n"
"   -t|--threads NUM  when and if the stream is launched, run at most\n"
"                     NUM threads.  The default is %" PRIu32 ". If this\n"
"               option is not given before a --run option this option\n"
"               will not effect that --run option.  On the Linux system\n"
"               the maximum number of threads a process may have can be\n"
"               gotten from running: cat /proc/sys/kernel/threads-max\n"
"\n"
"\n"
"   -V|--version  print %s version information to stdout an than exit.\n"
"\n"
"\n",

    argv0, DEFAULT_MAXTHREADS, argv0);


    return 1; // non-zero error code
}



int main(int argc, const char * const *argv) {

    signal(SIGSEGV, gdb_catcher);

    // This program will only construct one QsApp and one QsStream.

    struct QsApp *app = 0;
    struct QsStream *stream = 0;
    int numFilters = 0;
    struct QsFilter **filters = 0;
    bool verbose = false;
    bool ready = false;
    // TODO: option to change maxThreads.
    uint32_t maxThreads = DEFAULT_MAXTHREADS;
    char *endptr = 0;

    int i = 1;
    const char *arg = 0;

    while(i < argc) {

        const struct opts options[] = {
            { "connect", 'c' },
            { "display", 'd' },
            { "display-wait", 'D' },
            { "filter", 'f' },
            { "help", 'h' },
            { "help", '?' },
            { "plug", 'p' },
            { "ready", 'R' },
            { "run", 'r' },
            { "threads", 't' },
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
                //   --connect "0 1 1 2"
                //
                //           filter connections ==> (0) -> (1) -> (2)
                //

                if(!arg || arg[0] < '0' || arg[0] > '9') {
                    // There is no connection list, so by default we
                    // connect filters in the order they are loaded.
                    for(int i=1; i<numFilters; ++i) {
                        fprintf(stderr,"connecting: %d -> %d\n", i-1, i);
                        qsStreamConnectFilters(stream, filters[i-1],
                                filters[i], QS_NEXTPORT, QS_NEXTPORT);
                    }
                    break;
                }
                endptr = 0;
                for(const char *str = arg; *str && endptr != str; endptr = 0) {

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

                    qsStreamConnectFilters(stream, filters[from],
                                filters[to], 0, QS_NEXTPORT);
                }

                DSPEW("option %c = %s", c, arg);

                // next
                arg = 0;
                ++i;

                break;

            case 'p':
                if(numFilters < 2) {
                    fprintf(stderr, "Got --plug "
                            "option with %d filter(s) loaded\n",
                            numFilters);
                    return usage(argv[0]);
                }
                if(!arg) {
                    fprintf(stderr, "Bad --plug option\n\n");
                    return usage(argv[0]);
                }


                // Example:
                //
                //   --plug "0 1 1 2"
                //
                //           filter connections ==> (F_0:PORT_1) --> (F_1:PORT_2)
                //

                endptr = 0;
                const char *str = arg;

                long fromF = strtol(str, &endptr, 10);
                if(endptr == str) {
                    fprintf(stderr, "Bad --plug \"%s\" values "
                            "out of range\n\n", arg);
                    return usage(argv[0]);
                }
                str = endptr;

                long toF = strtol(str, &endptr, 10);
                if(endptr == str) {
                    fprintf(stderr, "Bad --plug \"%s\" values "
                            "out of range\n\n", arg);
                    return usage(argv[0]);
                }
                str = endptr;

                long fromPort = strtol(str, &endptr, 10);
                if(endptr == str) {
                    fprintf(stderr, "Bad --plug \"%s\" values "
                            "out of range\n\n", arg);
                    return usage(argv[0]);
                }
                str = endptr;

                long toPort = strtol(str, &endptr, 10);
                if(endptr == str) {
                    fprintf(stderr, "Bad --plug \"%s\" values "
                            "out of range\n\n", arg);
                    return usage(argv[0]);
                }
                str = endptr;

                // Check values read.
                if(fromF < 0 || toF < 0 || fromF >= numFilters
                        || toF >= numFilters) {
                    fprintf(stderr, "Bad --plug \"%s\" values "
                            "out of range\n\n", arg);
                    return usage(argv[0]);
                }

                fprintf(stderr, "connecting: %ld:%ld -> %ld:%ld\n",
                        fromF, fromPort, toF, toPort);

                qsStreamConnectFilters(stream, filters[fromF],
                            filters[toF], fromPort, toPort);

                DSPEW("option %c = %s", c, arg);

                // next
                arg = 0;
                ++i;

                break;

            case 'd':
                // display a dot graph and no wait
                if(!app) {
                    fprintf(stderr, "--display no filters loaded"
                            " to display\n");
                    break; // nothing to display yet.
                }
                qsAppDisplayFlowImage(app, QSPrintDebug,
                        false/*waitForDisplay*/);
                break;

             case 'D':
                // display a dot graph and wait
                if(!app) {
                    fprintf(stderr, "--display-wait no filters "
                            "loaded to display\n");
                    break; // nothing to display yet.
                }
                qsAppDisplayFlowImage(app, QSPrintDebug,
                        true/*waitForDisplay*/);
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
                ++i;

                if(i < argc && argv[i][0] == '[') {
                    if(argv[i][1] == '\0')
                        // --filter stdin [ --name ...
                        fargv = &argv[++i];
                    else
                        // --filter stdin [--name ...
                        fargv = &argv[i];

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
                }
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

            case 'R':

                if(ready) {
                    fprintf(stderr, "--ready with stream already ready\n");
                    return 1;
                } else if(!app) {
                    fprintf(stderr, "--ready with no filters loaded\n");
                    return 1;
                }

                if(qsStreamReady(stream)) {
                    // error
                    return 1;
                }

                // success
                ready = true;
                break;

            case 't':

                if(!arg) {
                    fprintf(stderr, "Bad --threads option\n\n");
                    return usage(argv[0]);
                }
                errno = 0;
                maxThreads = strtoul(arg, 0, 10);
                ++i;
                arg = 0;

                break;

            case 'r':

                if(!app) {
                    fprintf(stderr, "option --ready with no"
                            " filters loaded\n");
                    return 1;
                }

                if(!ready) {
                    if(qsStreamReady(stream))
                        // error
                        return 1;
                }

                if(qsStreamLaunch(stream, maxThreads)) {
                    // error
                    return 1;
                }

                if(qsStreamWait(stream))
                    return 1;

                if(qsStreamStop(stream))
                    return 1;

                ready = false;

                break;

            default:
                // This should not happen.
                fprintf(stderr, "unknown option character: %d\n", c);
                return usage(argv[0]);
        }
    }

    DASSERT(numFilters, "");

    DSPEW("Done parsing command-line arguments");

    return 0;
}
