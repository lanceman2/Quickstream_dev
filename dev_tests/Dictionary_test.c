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

static
size_t num_check = 0;
static
struct QsDictionary *d;


int callback(const char *key, const void *value) {

    ++num_check;
    
    fprintf(stderr, "%zu --- key=\"%s\" value=\"%s\"\n",
            num_check,
            key, (char *) value);

    ASSERT(value == qsDictionaryFind(d, key));

    return 0; // keep going.
}


int main(int argc, char **argv) {

    signal(SIGSEGV, catchSegv);

    d = qsDictionaryCreate();

    ASSERT(d);

    const char *keys[] = {
        "h", "h",
        "hay stack", "needle",
        "hay stace", "hay stace",
        "h2", "h2",
        "hay stack2", "hay stack2",
        "hay stace2", "hay stace2",
        "h21", "h2",
        "hay1 stack2", "hay stack2",
        "hay1 stace2", "hay stace2",


        0, 0
    };

    size_t num = 0;

    for(const char **key = keys; *key; ++key) {
        const char *val = *(key + 1);
        fprintf(stderr, "key=\"%s\", value=\"%s\"\n", *key, val);
        qsDictionaryInsert(d, *key, val);
        ++num;
        ++key;
    }


    qsDictionaryForEach(d, callback);

    ASSERT(num == num_check, "num=%zu  num_check=%zu",
            num, num_check);

    qsDictionaryPrintDot(d, stdout);


    if((isatty(1)))
        fprintf(stderr, "\nTry running: %s | display\n\n", argv[0]);

    qsDictionaryDestroy(d);

    fprintf(stderr, "SUCCESS\n");

    return 0;
}
