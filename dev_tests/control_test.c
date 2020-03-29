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



int main(int argc, char **argv) {

    signal(SIGSEGV, catcher);

    struct QsApp *app = qsAppCreate();
    ASSERT(app);
    struct QsStream *s = qsAppStreamCreate(app);
    ASSERT(s);

    int val = -33;

    qsParameterCreate(s, "wombat", "poo", &val, true);

    qsDictionaryPrintDot(app->dict, stdout);


    fprintf(stderr, "\nTry:  %s | display\n\n", argv[0]);


    fprintf(stderr, "SUCCESS\n");
    
    return 0;
}
