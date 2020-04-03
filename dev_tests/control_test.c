#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>

#include "../lib/debug.h"
#include "../include/quickstream/filter.h"
#include "../include/quickstream/app.h"
#include "../lib/qs.h"
#include "../lib/Dictionary.h"


static
void catcher(int sig) {
    fprintf(stderr, "Caught signal %u\n"
            "\n"
            "  Try:  gdb -pid %u\n\n"
            "Will now sleep ...\n",
            sig, getpid());
    while(1) {
        usleep(10000);
    }
}


#if 0
static int callback(void *val) {

    fprintf(stderr, "callback got freq=%d\n", *((int *) val));

    return 0;
}
#endif


static
int getCallback(
        void *value, const struct QsStream *stream,
        const char *filterName, const char *pName, 
        enum QsParameterType type) {

    fprintf(stderr, "control_test getCallback(value=%lg,"
            " stream=%p, \"%s\","
            "\"%s\", type=%" PRIu32 ")\n",
            *(double *) value, stream, filterName, pName,
            type);

    return 0;
}



int main(int argc, char **argv) {

    signal(SIGSEGV, catcher);

    struct QsApp *app = qsAppCreate();
    ASSERT(app);
    struct QsStream *s = qsAppStreamCreate(app);
    ASSERT(s);

    qsStreamFilterLoad(s, "tests/passThrough", "passThrough", 0, 0);

    ASSERT(qsParameterGet(s, "passThrough", "sleep",
                QsDouble, getCallback) == 0);

    ASSERT(qsParameterGet(s, "passThrough", "sleep",
                QsDouble, getCallback) == 0);

    ASSERT(qsParameterGet(s, "passThrough", "sleep",
                QsDouble, getCallback) == 0);

#if 1
    double t = 2.3;
    
    ASSERT(qsParameterSet(s, "passThrough",
                "sleep", QsDouble, &t) == 0);

#endif
    qsDictionaryPrintDot(app->dict, stdout);


    fprintf(stderr, "\nTry:  %s | display\n\n", argv[0]);


    fprintf(stderr, "SUCCESS\n");
    
    return 0;
}
