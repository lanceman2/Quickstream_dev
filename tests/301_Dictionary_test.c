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
        "0123", "0123",
        "01234", "0123",
        "0123456", "0123",
        "012345", "0123",
        "123", "0123",
        "1111111", "111111",
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
        "hay stack", "needle",
        "h", "h",
        "aa", "aa",
        "ab", "ab",
        "01", "01",
        "02", "02",
        "012", "012",

        "x40123", "0123",
        "x01234", "0123",
        "x0123456", "0123",
        "x012345", "0123",
        "x123", "0123",
        "x1111111", "111111",
        "x11111", "111111",
        "x1111x11", "111111",
        "x1111x111", "111111",
        "x1111z11", "111111",
        "x1111g11", "111111",
        "x1111", "111111",
        "xxr00123", "0123",
        "xwr00123", "0123",
        "xx00123", "0123",
        "x0", "0",
        "x0124", "0123",
        "x099", "099",
        "xfoo", "fooVal",
        "xbar", "barVal",
        "xboo", "boo",
        "xBar", "Bar",
        "xStar start3", "Star:start3Val",
        "xStar start", "poo",
        "xStar star", "poo",
        "xdog", "poo",
        "xcat", "poo",
        "x00", "0",
        "xcatdog", "poo   from catdog",
        "xhay stack", "needle",
        "xh", "h",
        "xaa", "aa",
        "xab", "ab",
        "x01", "01",
        "x02", "02",
        "x012", "012",
        "q", "q",

        0, 0
    };
#else
    const char *keys[] = {
        "0123", "0123",
        "01", "01",
        "11", "11",
        "023", "023",
        0, 0
    };
#endif

    for(const char **key = keys; *key; ++key) {
        const char *val = *(key + 1);
        ASSERT(qsDictionaryInsert(d, *key, val) == 0);
        fprintf(stderr, "added %s, %s\n", *key, val);
        ++key;
    }


    qsDictionaryPrintDot(d, stdout);


    for(const char **key = keys; *key; ++key) {
        const char *stored = qsDictionaryFind(d, *key);
        const char *val = *(key + 1);
        ASSERT(val == stored, "key=\"%s\" val=\"%s\" != stored=\"%s\"",
                *key, val, stored);
        fprintf(stderr, "key=\"%s\", val=\"%s\"\n", *key, val);
        ++key;
    }

    qsDictionaryDestroy(d); 

    return 0;
}
