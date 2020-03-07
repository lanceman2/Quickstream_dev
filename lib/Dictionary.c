#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "Dictionary.h"
#include "../lib/debug.h"

#define ALPHABET_SIZE  ((size_t) 62)

#if 0
static
const char *Alphabet = "abcdefghijklmnopqrstuvwxyz"
                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                       "0123456789";
#endif

// A Dictionary via a trie
// References:
// https://www.geeksforgeeks.org/trie-insert-and-search/
// https://en.wikipedia.org/wiki/Trie
//
struct QsDictionary {

    // For our use case:
    //   - We do not need fast insert.
    //   - We do not need remove at all.
    //   - Just need fast lookup.
    //   - This will hold about 1000 elements in extreme cases.
    //
    struct QsDictionary *children[ALPHABET_SIZE];

    bool isEndOfWord;
};


struct QsDictionary *qsDictionaryCreate(void) {

    struct QsDictionary *d = calloc(1, sizeof(*d));
    ASSERT(d, "calloc(1,%zu) failed", sizeof(*d));

    return d;
}


void qsDictionaryDestroy(struct QsDictionary *dict) {

    DASSERT(dict);

    free(dict);
}


// Returns ptr.
void *qsDictionaryInsert(struct QsDictionary *node,
        const char *key, const void *value) {

    DASSERT(node);

    

    return (void *) value;
}


// Returns element ptr.
void *qsDictionaryFind(struct QsDictionary *dict, const char *key) {


    return 0;
}
