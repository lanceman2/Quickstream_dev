#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <inttypes.h>
#include "../lib/Dictionary.h"
#include "../lib/debug.h"


void catchSegv(int sig) {
    fprintf(stderr, "\nCaught signal %d\n"
            "\nsleeping:  gdb -pid %u\n",
            sig, getpid());
    while(true) usleep(100000);
}


int main(int argc, char **argv) {

    signal(SIGSEGV, catchSegv);

    struct QsDictionary *d = qsDictionaryCreate();

    const char *keys[] = {
        "h", "hello",
        "hay stack", "needle",
        "hay stace", "hello2",
        "hay stace", "hello2",
        "h", "hello",



        0, 0
    };

    for(const char **key = keys; *key; ++key) {
        const char *val = *(key + 1);
        fprintf(stderr, "key=\"%s\", value=\"%s\"\n", *key, val);
        qsDictionaryInsert(d, *key, val);
        ++key;
    }


    qsDictionaryPrintDot(d, stdout);


    if((isatty(1)))
        fprintf(stderr, "\nTry running: %s | display\n\n", argv[0]);

    qsDictionaryDestroy(d);

    fprintf(stderr, "SUCCESS\n");

    return 0;
}
