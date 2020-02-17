/** \page quickstream_program quickstream
 
\tableofcontents

The quickstream command line program.

\section quickstream_intro Introduction

\htmlinclude quickstreamIntro.htm

\section quickstream_options Command Line Options

\htmlinclude quickstreamOptions.htm


\section quickstream_examples Examples

\subsection quickstream_helloworld Hello World

With quickstream installed and quickstream in your PATH you can run:

\code{.sh}
echo "Hello World!" | quickstream --filter stdin --filter stdout --connect --run
\endcode

You can view a graphviz dot image of the flow graph related to the above
program that we ran above by running:
\code{.sh}
quickstream --filter stdin --filter stdout --connect --display
\endcode

\image html stdinStdout.png

*/



#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../include/quickstream/app.h"
#include "../lib/debug.h"
#include "getOpt.h"
#include "../lib/quickstream/misc/qsOptions.h"



static void gdb_catcher(int signum) {

    ASSERT(0, "Caught signal %d\n", signum);
}


static
int usage(int fd) {

    const char *run = "lib/quickstream/misc/quickstreamHelp";
    ssize_t postLen = strlen(run);
    ssize_t bufLen = 128;
    char *buf = (char *) malloc(bufLen);
    ASSERT(buf, "malloc(%zu) failed", bufLen);
    ssize_t rl = readlink("/proc/self/exe", buf, bufLen);
    ASSERT(rl > 0, "readlink(\"/proc/self/exe\",,) failed");
    while(rl + postLen >= bufLen)
    {
        DASSERT(bufLen < 1024*1024); // it should not get this large.
        buf = (char *) realloc(buf, bufLen += 128);
        ASSERT(buf, "realloc() failed");
        rl = readlink("/proc/self/exe", buf, bufLen);
        ASSERT(rl > 0, "readlink(\"/proc/self/exe\",,) failed");
    }

    buf[rl] = '\0';

    // Now we have something like:
    // buf = "/usr/local/foo/bin/quickstream"

    // remove the "quickstream" part
    ssize_t l = rl - 1;
    // move to a '/'
    while(l && buf[l] != '/') --l;
    if(l) --l;
    // move to another '/'
    while(l && buf[l] != '/') --l;
    if(l == 0) {
        printf("quickstreamHelp was not found\n");
        return 1;
    }

    // now buf[l] == '/'
    // add the "lib/quickstream/misc/quickstreamHelp"
    strcpy(&buf[l+1], run);

    //printf("running: %s\n", buf);

    if(fd != STDOUT_FILENO)
        // Make the quickstreamHelp write to stderr.
        dup2(fd, STDOUT_FILENO);

    execl(buf, buf, "-h", NULL);

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

        int c = getOpt(argc, argv, &i, qsOptions, &arg);

        switch(c) {

            case -1:
            case '*':
                fprintf(stderr, "Bad option: %s\n\n", arg);
                return usage(STDERR_FILENO);

            case '?':
            case 'h':
                return usage(STDOUT_FILENO);

            case 'V':
                printf("%s\n", QS_VERSION);
                return 0;

            case 'v':
                verbose = true;
                break;

            case 'n':
                verbose = false;
                break;

            case 'c':
                if(numFilters < 2) {
                    fprintf(stderr, "Got --connect "
                            "option with %d filter(s) loaded\n",
                            numFilters);
                    return usage(STDERR_FILENO);
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
                        return usage(STDERR_FILENO);
                    }
                    str = endptr;

                    long to = strtol(str, &endptr, 10);
                    if(endptr == str) {
                        fprintf(stderr, "Bad --connect \"%s\" values "
                                "out of range\n\n", arg);
                        return usage(STDERR_FILENO);
                    }
                    str = endptr;

                    // Check values read.
                    if(from < 0 || to < 0 || from >= numFilters
                            || to >= numFilters) {
                        fprintf(stderr, "Bad --connect \"%s\" values "
                                "out of range\n\n", arg);
                        return usage(STDERR_FILENO);
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
                    return usage(STDERR_FILENO);
                }
                if(!arg) {
                    fprintf(stderr, "Bad --plug option\n\n");
                    return usage(STDERR_FILENO);
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
                    return usage(STDERR_FILENO);
                }
                str = endptr;

                long toF = strtol(str, &endptr, 10);
                if(endptr == str) {
                    fprintf(stderr, "Bad --plug \"%s\" values "
                            "out of range\n\n", arg);
                    return usage(STDERR_FILENO);
                }
                str = endptr;

                long fromPort = strtol(str, &endptr, 10);
                if(endptr == str) {
                    fprintf(stderr, "Bad --plug \"%s\" values "
                            "out of range\n\n", arg);
                    return usage(STDERR_FILENO);
                }
                str = endptr;

                long toPort = strtol(str, &endptr, 10);
                if(endptr == str) {
                    fprintf(stderr, "Bad --plug \"%s\" values "
                            "out of range\n\n", arg);
                    return usage(STDERR_FILENO);
                }
                str = endptr;

                // Check values read.
                if(fromF < 0 || toF < 0 || fromF >= numFilters
                        || toF >= numFilters) {
                    fprintf(stderr, "Bad --plug \"%s\" values "
                            "out of range\n\n", arg);
                    return usage(STDERR_FILENO);
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

            case 'g':
                // display a dot graph and no wait
                if(!app) {
                    fprintf(stderr, "--dot no filters loaded"
                            " to display\n");
                    break; // nothing to display yet.
                }
                qsAppPrintDotToFile(app, QSPrintDebug, stdout);
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
                    return usage(STDERR_FILENO);
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
                //  --filter stdin { --name stdinput --bufferSize 5012 }
                //
                int fargc = 0; // 4
                const char * const *fargv = 0; // points to --name
                ++i;

                if(i < argc && argv[i][0] == '{') {
                    if(argv[i][1] == '\0')
                        // --filter stdin [ --name ...
                        fargv = &argv[++i];
                    else
                        // --filter stdin [--name ...
                        fargv = &argv[i];

                    while(i < argc && strcmp(argv[i], "}")) {
                        if(strcmp(argv[i],"--name") == 0) {
                            ++i;
                            ++fargc;
                            if(i < argc && strcmp(argv[i], "}")) {
                                name = argv[i];
                                ++i;
                                ++fargc;
                            }
                        } else {
                            ++i;
                            ++fargc;
                        }
                    }
                    if(strcmp(argv[i], "}") == 0) ++i;
                }
                if(verbose) {
                    fprintf(stderr, "Got filter args[%d]= {", fargc);
                    for(int j=0; j<fargc; ++j)
                        fprintf(stderr, "%s ", fargv[j]);
                    fprintf(stderr, "}\n");
                }

                filters[numFilters-1] = qsAppFilterLoad(app,
                        arg, name, fargc, (const char **) fargv);
                if(!filters[numFilters-1]) return 1; // error

                // next
                arg = 0;

                break;


            case 'F':
                if(!arg) {
                    fprintf(stderr, "Bad --filter-help option\n\n");
                    return usage(STDERR_FILENO);
                }
                return qsFilterPrintHelp(arg, stdout);


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
                    return usage(STDERR_FILENO);
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
                return usage(STDERR_FILENO);
        }
    }

    DSPEW("Done parsing command-line arguments");

    return 0;
}
