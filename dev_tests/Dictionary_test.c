#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

#include "../lib/Dictionary.h"
#include "../lib/debug.h"


void catchSegv(int sig) {
    fprintf(stderr, "\nCaught signal %d\n"
            "\nsleeping:  gdb -pid %u\n",
            sig, getpid());
    while(true) usleep(100000);
}


int main(void) {

    signal(SIGSEGV, catchSegv);

    struct QsDictionary *d = qsDictionaryCreate();

#if 1
    const char *keys[] = {
        "0123", "",
        "099", "",
        "foo", "fooVal",
        "bar", "barVal",
        "Bar", "",
        "Star start3", "Star:start3Val",
        "Star start", "poo",
        "Star star", "poo",
        "dog", "poo",
        "cat", "poo",
        "catdog", "poo   from catdog",
        "hay stack", "needle",
        "a", "b",
        "aa", "aa",
        "ab", "ab",
        "01", "",
        "02", "",
        "012", "",
        "01234", "",
        0, 0
    };
#else
    const char *keys[] = {
        "0123", "",
        "099", "",
        "099", "",
        0, 0
    };
#endif



    for(const char **key = keys; *key; ++key) {
        const char *val = *(key + 1);
        ASSERT(qsDictionaryInsert(d, *key, val) == 0);
        fprintf(stderr, "adding %s, %s\n", *key, val);
        ++key;
    }

    qsDictionaryPrintDot(d, stdout);

    qsDictionaryDestroy(d); 

    return 0;
}
