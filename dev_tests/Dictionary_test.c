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

#define START          (32) // start at SPACE
#define END            (126) // ~

#if 0
    char key[5];
    key[3] = 0;
    key[4] = 0;
    void *val = 0;

    for(int i = START; i<=END; ++i) {
        key[0] = i;
        for(int j = END; j>=START; --j) {
            key[1] = j;
            for(int k = START; k<=START+2; ++k) {
                key[2] = k;
                val = (void *) (uintptr_t) (i + (j * END) +  k * END * END);
                fprintf(stderr, "key=\"%s\", value=\"%p\"\n", key, val);
                ASSERT(qsDictionaryInsert(d, key, val) == 0);
            }
        }
    }
    key[3] = 'r';
    for(int i = START; i<=END; ++i) {
        key[1] = i;
        for(int j = END; j>=START; --j) {
            key[0] = j;
            for(int k = START; k<=START+2; ++k) {
                key[2] = k;
                val = (void *) (uintptr_t) (i + (j * END) +  k * END * END);
                fprintf(stderr, "key=\"%s\", value=\"%p\"\n", key, val);
                ASSERT(qsDictionaryInsert(d, key, val) == 0);
            }
        }
    }
    key[2] = 'r';
    for(int i = START; i<=END; ++i) {
        key[1] = i;
        for(int j = END; j>=START; --j) {
            key[0] = j;
            for(int k = START; k<=START+2; ++k) {
                key[3] = k;
                val = (void *) (uintptr_t) (i + (j * END) +  k * END * END);
                fprintf(stderr, "key=\"%s\", value=\"%p\"\n", key, val);
                ASSERT(qsDictionaryInsert(d, key, val) == 0);
            }
        }
    }
#else
    const char *keys[] = {
        "heal", "hello",
        "hay stack", "needle",

        "kea", "kea",
        "keb", "keb",
        "healo", "hello",

#if 1
        "0123", "0123",
        "01234", "0123",
        "0123456", "0123",
        "012345", "0123",
        "123", "0123",
        "11", "111111",
        "11111", "111111",
        "1111x11", "111111",
        "1111x111", "111111",
        "1111z11", "111111",
        "1111g11", "111111",
        "1111", "111111",
        "x00123", "0123",
        "w00123", "0123",
        "00123", "0123",
        "0", "0",
        "0124", "0123",
        "099", "099",
        "foo", "fooVal",
        "bar", "barVal",
        "boo", "boo",
        "Bar", "Bar",
        "Star start3", "Star:start3Val",
        "Star start", "poo",
        "Star star", "poo",
        "dog", "poo",
        "cat", "poo",
        "00", "0",
        "catdog", "poo   from catdog",
        "h", "h",
        "aa", "aa",
        "ab", "ab",
        "01", "01",
        "02", "02",
        "012", "012",
        "q", "q",

        "!0123", "0123",
        "!01234", "0123",
        "!0123456", "0123",
        " 012345", "0123",
        " 123", "0123",
        " 11", "111111",
        " 11111", "111111",
        " 1111x11", "111111",
        " 1111x111", "111111",
        " 1111z11", "111111",
        " 1111g11", "111111",
        " 1111", "111111",
        " x00123", "0123",
        " w00123", "0123",
        " 00123", "0123",
        "0 ", "0",
        " 0124", "0123",
        " 099", "099",
        " foo", "fooVal",
        " bar", "barVal",
        " boo", "boo",
        " Bar", "Bar",
        " Star start3", "Star:start3Val",
        " Star start", "poo",
        " Star star", "pooi",
        " dog", "poo",
        " cat", "poo",
        " 00", "0",
        "~catdog", "poo   from catdog",
        "~h", "h",
        "~aa", "aa",
        "~ab", "ab",
        "~01", "01",
        "~02", "02",
        "~012", "012",
        "~q", "q",

#  endif
        0, 0
    };

    for(const char **key = keys; *key; ++key) {
        const char *val = *(key + 1);
        fprintf(stderr, "key=\"%s\", value=\"%s\"\n", *key, val);
        ASSERT(qsDictionaryInsert(d, *key, val) == 0);
        ++key;
    }

#endif

#if 1
    for(const char **key = keys; *key; ++key) {
        const char *val = *(key + 1);
        fprintf(stderr, "key=\"%s\", value=\"%s\"\n", *key, val);
        ASSERT(val == qsDictionaryFind(d, *key));
        ++key;
    }
#endif


    qsDictionaryPrintDot(d, stdout);

    qsDictionaryDestroy(d);

    if((isatty(1)))
        fprintf(stderr, "\nTry running: %s | display\n\n", argv[0]);

    fprintf(stderr, "SUCCESS\n");

    return 0;
}
