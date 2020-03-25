/** \page quickstream_program quickstream
 
\tableofcontents

The quickstream command line program.

\section quickstream_intro Usage

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
#include <stdatomic.h>

#include "../include/quickstream/app.h"
#include "../lib/debug.h"
#include "getOpt.h"
#include "../lib/quickstream/misc/qsOptions.h"

    
// The spew level.
static int level = 1; // 0 == no spew, 1 == error, ...


static int numStreams = 0;
// array of streams pointers of length numStreams.
static struct QsStream **streams = 0; 


static void gdb_catcher(int signum) {

    // The action of ASSERT() depends on how this is compiled.
    ASSERT(0, "Caught signal %d\n", signum);
}

static void term_catcher(int signum) {

    if(level >= 3) // notice
        fprintf(stderr, "Caught signal %d, stopping source feeds\n",
                signum);
    for(int i=0; i<numStreams; ++i)
        qsStreamStopSources(streams[i]);

    // Let the user kill the program if it's not rerunning.
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);
}


static
int usage(int fd) {

    // This usage() is a little odd.  It launches another program to
    // display the program Usage.  Why?  We put the --help, man pages,
    // together in one program that is also used to generate some of the
    // code that are this program.  This keeps the documentation of the
    // program and it's options consistent.  Really most of the code of
    // this program is in the library libquickstream.so.  The program
    // quickstream should be just a command-line wrapper of
    // libquickstream.so, if that is not the case than we are doing
    // something wrong.

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

    //fprintf(stderr, "running: %s\n", buf);

    if(fd != STDOUT_FILENO)
        // Make the quickstreamHelp write to stderr.
        dup2(fd, STDOUT_FILENO);

    execl(buf, buf, "-h", NULL);

    fprintf(stderr, "execl(\"%s\",,) failed\n", buf);

    return 1; // non-zero error code, fail.
}



int main(int argc, const char * const *argv) {

    // Hang the program for debugging, if we segfault.
    signal(SIGSEGV, gdb_catcher);

    // This program will only construct one QsApp, but can make an number
    // of QsStream, streams.

    struct QsApp *app = 0; // The one and other for this program.

    struct QsStream *stream = 0; // The last stream created.

    int numMaxThreads = 1;
    // array of maxThreads of length numStreams.
    // There can be one maxThreads for each stream in the stream array.
    uint32_t *maxThreads = malloc(numMaxThreads * sizeof(*maxThreads));
    ASSERT(maxThreads, "malloc(%zu) failed",
            numMaxThreads * sizeof(*maxThreads));
    maxThreads[0] = DEFAULT_MAXTHREADS;

    int numFilters = 0;
    struct QsFilter **filters = 0;
    bool ready = false;
    // TODO: option to change maxThreads.
    char *endptr = 0;

    int i = 1;
    const char *arg = 0;

    qsSetSpewLevel(DEFAULT_SPEW_LEVEL);

    while(i < argc) {

        int c = getOpt(argc, argv, &i, qsOptions, &arg);

        switch(c) {

            case -1:
            case '*':
                fprintf(stderr, "Bad option: %s\n\n", arg);
                return usage(STDERR_FILENO);

            case '?':
            case 'h':
                // The --help option get stdout and all the error
                // cases get stderr.
                return usage(STDOUT_FILENO);

            case 'V':
                printf("%s\n", QS_VERSION);
                return 0;

            case 'v':

                if(!arg) {
                    fprintf(stderr, "--verbose with no level\n");
                    return usage(STDERR_FILENO);
                }
                {
                    // LEVEL maybe debug, info, notice, warn, error, and
                    // off which translates to: 5, 4, 3, 2, 1, and 0
                    // as this program (and not the API) define it.
                    if(*arg == 'd' || *arg == 'D' || *arg == '5')
                        // DEBUG 5
                        level = 5;
                    else if(*arg == 'i' || *arg == 'I' || *arg == '4')
                        // INFO 4
                        level = 4;
                    else if(*arg == 'n' || *arg == 'N' || *arg == '3')
                        // NOTICE 3
                        level = 3;
                    else if(*arg == 'w' || *arg == 'W' || *arg == '2')
                        // WARN 2
                        level = 2;
                    else if(*arg == 'e' || *arg == 'E' || *arg == '1')
                        // ERROR 1
                        level = 1;
                    else // none and anything else
                        // NONE 0 with a error string.
                        level = 0;

                    if(level >= 3/*notice*/)
                        fprintf(stderr, "quickstream spew level "
                                "set to %d\n"
                                "The highest libquickstream "
                                "spew level is %d\n",
                                level, qsGetLibSpewLevel());

                    qsSetSpewLevel(level);

                    // next
                    arg = 0;
                    ++i;
                    break;
                }

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
                        if(level >= 4)
                            fprintf(stderr,"connecting: %d -> %d\n", i-1, i);
                        qsFiltersConnect(filters[i-1],
                                filters[i], QS_NEXTPORT, QS_NEXTPORT);
                    }
                    break;
                }

                endptr = 0;
                for(const char *str = arg; *str && endptr != str;
                        endptr = 0) {

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

                    if(level >= 4)
                        fprintf(stderr, "connecting: %ld -> %ld\n",
                                from, to);

                    qsFiltersConnect(filters[from],
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
                //  filter connections ==> (F_0:PORT_1) --> (F_1:PORT_2)
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

                if(level >= 4)
                    fprintf(stderr, "connecting: %ld:%ld -> %ld:%ld\n",
                            fromF, fromPort, toF, toPort);

                qsFiltersConnect(filters[fromF],
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
                    ++numStreams;
                    streams = realloc(streams, numStreams * sizeof(*streams));
                    ASSERT(streams, "realloc(, %zu) failed",
                            numStreams * sizeof(*streams));
                    streams[numStreams-1] = stream;
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

                if(level >= 5 && fargc) {
                    fprintf(stderr, "Got filter args[%d]= {", fargc);
                    for(int j=0; j<fargc; ++j)
                        fprintf(stderr, "%s ", fargv[j]);
                    fprintf(stderr, "}\n");
                }

                filters[numFilters-1] = qsStreamFilterLoad(stream,
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
                    fprintf(stderr, "--ready with stream already"
                            " ready\n");
                    return 1;
                } else if(!app) {
                    fprintf(stderr, "--ready with no filters loaded\n");
                    return 1;
                }

                if(level >= 4/*info*/)
                    fprintf(stderr, "Readying %d streams\n", numStreams);

                for(int j=0; j<numStreams; ++j)
                    if(qsStreamReady(streams[j]))
                        // error
                        return 1;

                // success
                ready = true;
                break;

            case 't':

                if(!arg) {
                    fprintf(stderr, "Bad --threads option\n\n");
                    return usage(STDERR_FILENO);
                }
                
                {
                    int num = 0;
                    errno = 0;
                    endptr = 0;
                    for(const char *str = arg; *str && endptr != str;) {
                        long val = strtol(str, &endptr, 10);
                        if(endptr == str) break;
                        ++num;
                        if(num > numMaxThreads) {
                            ++numMaxThreads;
                            maxThreads = realloc(maxThreads,
                                    numMaxThreads * sizeof(*maxThreads));
                            ASSERT(maxThreads, "realloc(%zu) failed",
                                    numMaxThreads * sizeof(*maxThreads));
                        }
                        maxThreads[num-1] = val;
                        str = endptr;
                    }
                }

                ++i;
                arg = 0;

                break;

            case 's':

                ++numStreams;

                if(!app)
                    app = qsAppCreate();
                ASSERT(app);

                if(numStreams > 1) {
                    streams = realloc(streams,
                            numStreams * sizeof(*streams));
                    ASSERT(streams, "realloc(%zu) failed",
                            numStreams * sizeof(*streams));
                }

                stream = streams[numStreams-1] = qsAppStreamCreate(app);
                ASSERT(stream);

                break;

            case 'r': // --run

                if(!app) {
                    fprintf(stderr, "option --ready with no"
                            " filters loaded\n");
                    return 1;
                }

                if(!ready)
                    for(int j=0; j<numStreams; ++j)
                        if(qsStreamReady(stream))
                            // error
                            return 1;

                ready = true;
                signal(SIGTERM, term_catcher);
                signal(SIGINT, term_catcher);

                if(level >= 4)
                    fprintf(stderr, "Running stream\n");

                int max_threads = maxThreads[0];
                for(int j=0; j<numStreams; ++j) {
                    if(j < numMaxThreads)
                        max_threads = maxThreads[j];
                    if(qsStreamLaunch(streams[j], max_threads))
                        // error
                        return 1;
                }

                max_threads = maxThreads[0];
                // We do not wait if there where no worker threads, that
                // is maxThreads == 0.
                for(int j=0; j<numStreams; ++j) {
                    if(j < numMaxThreads)
                        max_threads = maxThreads[j];
                    if(max_threads && qsStreamWait(streams[j]))
                        return 1; // error
                }

                for(int j=0; j<numStreams; ++j)
                    if(qsStreamStop(streams[j]))
                        return 1; // error

                if(level >= 4)
                    fprintf(stderr, "Finished running stream\n");

                ready = false;

                break;

            default:
                // This should not happen.
                fprintf(stderr, "unknown option character: %d\n", c);
                return usage(STDERR_FILENO);
        }
    }

    DSPEW("Done parsing command-line arguments");

    if(app)
        // This will destroy all streams.
        return qsAppDestroy(app);

    return 0;
}
