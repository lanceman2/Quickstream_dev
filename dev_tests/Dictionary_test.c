#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

#include "../lib/Dictionary.h"


void catchSegv(int sig) {
    fprintf(stderr, "\nCaught signal %d\n"
            "\nsleeping:  gdb -pid %u\n",
            sig, getpid());
    while(true) usleep(100000);
}


int main(void) {

    signal(SIGSEGV, catchSegv);

    struct QsDictionary *d = qsDictionaryCreate();

    const char *keys[] = {
        "foo", "fooVal",
        "bar", "barVal",
        "Bar", 0,
        "Star:start3", "Star:start3Val",
        "Star:start", "poo",
        "dog", "poo",
        "cat", "poo",
        "catdog", "poo   from catdog",
        "hay stack", "needle",
        "a", "b",
        "aa", "aa",
        "ab", "ab",
        0, 0
    };


    for(const char **key = keys; *key; ++key) {
        const char *val = *(key + 1);
        qsDictionaryInsert(d, *key, val);
        printf("%s, %s\n", *key, val);
        ++key;
    }

    qsDictionaryDestroy(d); 

    return 0;
}
