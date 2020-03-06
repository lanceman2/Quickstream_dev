#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "Dictionary.h"
#include "../lib/debug.h"

// A Dictionary Node.
struct QsDictionary {

    // This dictionary is a Trie tree data structure:
    // Reference: https://en.wikipedia.org/wiki/Trie
    //
    // For our use case:
    //   - We do not need fast insert.
    //   - We do not need remove at all.
    //   - Just need fast lookup.
    //   - This will hold about 1000 elements in extreme cases.
    //   - We assume that keys are unique to start with.
    //

    // children is a realloc() allocated array that is 0 terminated.
    // or 0 if there are no children.
    struct QsDictionary *children;

    void *value;

    const char *key;

    char c; // The character in this node
};


struct QsDictionary *qsDictionaryCreate(void) {

    struct QsDictionary *d = calloc(1, sizeof(*d));
    ASSERT(d, "calloc(1,%zu) failed", sizeof(*d));

    return d;
}


static void FreeChildren(struct QsDictionary *dict) {

    // Free children's children if they exist.
    for(struct QsDictionary *child = dict->children; child; ++child)
        if(child->children)
            FreeChildren(child);

    // We checked before that this "dict" has children.
    free(dict->children);
}


void qsDictionaryDestroy(struct QsDictionary *dict) {

    DASSERT(dict);

    // Free children's children if they exist.
    for(struct QsDictionary *child = dict->children; child; ++child)
        if(child->children)
            FreeChildren(child);

    if(dict->children)
        free(dict->children);
    free(dict);
}



// Returns ptr.
void *qsDictionaryInsert(struct QsDictionary *node,
        const char *key, const void *value) {

    DASSERT(node);
    DASSERT(key);
    DASSERT(key[0]);

    struct QsDictionary *child;
    size_t nlen = 2;

    // First find the node that will be the parent of the new node that we
    // will create here.
    //
    for(const char *c = key; *c; ++c) {
        // The new length (nlen) would add 2 more, one for the new child
        // and one for the 0 terminator node.
        nlen = 2;
        for(child = node->children; child; ++child) {
            if(*c == child->c)
                break;
            ++nlen;
        }

        if(child == 0) {

            child = realloc(node->children, nlen*sizeof(*child));
            ASSERT(child, "realloc(%p,%zu) failed",
                    node->children, nlen*sizeof(*child));
            // 0 terminate it.
            child[nlen-1] = 0;
            // Reuse "child" as the new node that is this child.
            child = &child[nlen-2];
            child->children = 0;
            child->c = *c;
            child->value = value;
            child->key = strdup(c+1);
        }

        node = child;
    }

    return (void *) value;
}


struct QsDictionary *_qsDictionaryFind(struct QsDictionary *d,
    const char *key) {



    return 0;
}


// Returns element ptr.
void *qsDictionaryFind(struct QsDictionary *dict, const char *key) {


    return dict->value;
}
