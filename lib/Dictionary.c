#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "Dictionary.h"
#include "../lib/debug.h"

//  126  ~      is the last nice looking ASCII char
//   32  SPACE  is the first nice looking ASCII char
#define START          (32) // start at SPACE
#define END            (126)
#define ALPHABET_SIZE  (END + 1 - START) // 95 pointers.
// We do not allow the any other characters.

// We'll store 95 * 8 + 8 Bytes per entry plus the part of the
// size of the string entry,
// = 768 Bytes per entry + the part of the strings
// For 200 strings that's 153600 Bytes plus ~ 154 kBytes
// But 

// TODO: We could consider more complex alphabet mapping that uses just
// the characters that are in the strings that we are storing.  Currently
// our mapping from character space to dictionary pointer is a simple
// constant offset.  We may be able to cut memory usage to one third of
// the current, at a relative cost of a more complex character space to
// dictionary pointer mapping.


// A Dictionary via a big fat Trie
//
// Faster than a hash table, but eats memory like a pig.
//
// References:
// We use something with less memory than in this:
// https://www.geeksforgeeks.org/trie-insert-and-search/
//
// https://en.wikipedia.org/wiki/Trie
//
struct QsDictionary {

    // A Dictionary node.

    // For our use case:
    //   - We do not need fast insert.
    //   - We do not need remove at all.
    //   - Just need fast lookup.
    //   - This will hold about 1000 elements in extreme cases.

    // If there are children than it's an array of length ALPHABET_SIZE.
    // otherwise it's 0.  We add a little complexity by having Null
    // children to start with.
    struct QsDictionary *children;

    // suffix is a string added to the current path traversal
    // generated string.  It can be 0 if no chars are needed.
    //
    // If an insert is done making the suffix become incorrect the node
    // must be broken, this node will loose as much of the extra
    // characters as it needs to and add children as it needs to, to make
    // all the traversals correct.  This helps us use much less memory,
    // by having more information in a node.  This little addition to the
    // "standard" Trie data structure adds a little complexity to
    // Insert(), but not much to Find().  We don't bother making "" empty
    // strings; we just make them 0 pointers.
    //
    const char *suffix;

    // value stored if there is one.  The key for this value is the string
    // generated by traversing the graph stringing together the characters
    // and optional suffix (if present) at each node in the traversal.
    // Sometimes this node is just a branch point with no value.
    const void *value;
};


struct QsDictionary *qsDictionaryCreate(void) {

    struct QsDictionary *d = calloc(1, sizeof(*d));
    ASSERT(d, "calloc(1,%zu) failed", sizeof(*d));

    struct QsDictionary *children =
        calloc(ALPHABET_SIZE, sizeof(*children));
    ASSERT(children, "calloc(%d,%zu) failed",
            ALPHABET_SIZE, sizeof(*children));
    d->children = children;

    return d;
}


static
void FreeChildren(struct QsDictionary *children) {

    for(struct QsDictionary *i = children + ALPHABET_SIZE - 1;
            i >= children; --i)
        if(i->children)
            // Recurse
            FreeChildren(i->children);
#if DEBUG
    memset(children, 0, ALPHABET_SIZE*sizeof(*children));
#endif
    free(children);
}


void qsDictionaryDestroy(struct QsDictionary *dict) {

    DASSERT(dict);

    if(dict->children)
        FreeChildren(dict->children);

#if DEBUG
    memset(dict, 0, sizeof(*dict));
#endif
    free(dict);
}


// Returns 0 on success or 1 if already present and -1 if it is not added
// and have an invalid character.
int qsDictionaryInsert(struct QsDictionary *node,
        const char *key, const void *value) {

    DASSERT(node);
    DASSERT(key);
    DASSERT(*key);
    DASSERT(value);

    for(const char *c = key; *c;) {

        // Checking the key characters along the way.
        if(*c < START || *c > END) {
            ERROR("Invalid character in key: \"%s\"",
                    key);
            return -1;
        }

        if(node->children) {
            // We will go to next child in the traversal.
            //
            // Go to the next child in the traversal.
            node = node->children + (*c) - START;

            if(node->suffix) {
                // We have a suffix in this node.
                //
                // Set up two sets of character iterators.
                const char *cc = c + 1; // one for the inserter
                const char *ee = node->suffix; // one for current suffix

                // Find the point where key and suffix do not match.
                for(;*ee && *cc && *ee == *cc; ++cc, ++ee)
                    if(*cc < START || *cc > END) {
                        // Checking the key characters along the way.
                        ERROR("Invalid character in key: \"%s\"",
                                key);
                        // Nothing changed at this point so we can just
                        // return -1;
                        return -1; // fail
                    }

                // CASES:
                //
                //  The only way to understand this is to draw pictures
                //  like you are a 6 year old kid.  Or maybe a 3 year old
                //  for all you prodigies.
                //
                // 1. (*ee == '\0')
                //    suffix ran out
                //    => MATCH or next node
                //
                // 2. (*cc == '\0')
                //    key ran out
                //    => INSERT split the node adding one children set
                //
                // 3. (*ee && ee != node->suffix)
                //    neither ran out and they do not match
                //    => BIFURCATE making two child nodes
                //

                if(*ee == '\0') {
                    // 1. MATCH  so far.
                    c = cc;
                    continue;
                }

                if(*cc == '\0') { // There are unmatched chars in suffix
                    // 2. INSERT
                    //
                    // We matched part way through the suffix.
                    //
                    // So: split the node in two and the first one has the
                    // new inserted value and the second has the old value
                    // and the old children.
                    //
                    // New node children:
                    struct QsDictionary *nchildren =
                            calloc(ALPHABET_SIZE, sizeof(*nchildren));
                    ASSERT(nchildren, "calloc(%d,%zu) failed",
                            ALPHABET_SIZE, sizeof(*nchildren));

                    // parent -> node1 (firstChar + start of suffix) ->
                    //   node2 (ee* + rest of suffix) -> ...

                    // dummy pointer node will be acting as node1.

                    struct QsDictionary *node2 = nchildren + (*ee) - START;
                    // node2 has all the children of node and the old
                    // value and the remaining suffix that was in node.
                    node2->children = node->children;
                    node2->value = node->value;
                    // node2 just has part of the old suffix.
                    if(*(ee+1)) {
                        node2->suffix = strdup(ee+1);
                        ASSERT(node2->suffix, "strdup(%p) failed", ee+1);
                    }

                    const char *oldSuffix = node->suffix;
                    // Now remake node
                    node->value = value; // it has the new value.
                    if(ee == node->suffix) {
                        // There where no matching chars in suffix
                        // and the char pointers never advanced.
                        node->suffix = 0;
                    } else {
                        // At least one char matched in suffix.  Null
                        // terminate the suffix after the last matching
                        // char and then dup it, and free the old remains
                        // of suffix.
                        * (char *) ee = '\0';
                        node->suffix = strdup(oldSuffix);
                        ASSERT(node->suffix, "strdup(%p) failed",
                                oldSuffix);
                    }
                    free((char *)oldSuffix);
                    // This node now gets the new children and has one
                    // child that is node2 from above.
                    node->children = nchildren;
                    return 0; // success
                }

                // BIFURCATE
                //
                // We match up to "ee" but there is more to go and we
                // don't match after that.  Result is we need to add 2
                // nodes.
                 if(node->children == 0) {
        // The root node starts with no children and here we make the
        // root family.
        struct QsDictionary *children =
                calloc(ALPHABET_SIZE, sizeof(*children));
        ASSERT(children, "calloc(%d,%zu) failed",
                ALPHABET_SIZE, sizeof(*children));
        node->children = children;
    }

   //
                // New node children:
                //
                // 2 of these children will be used in this bifurcation.
                struct QsDictionary *nchildren =
                        calloc(ALPHABET_SIZE, sizeof(*nchildren));
                ASSERT(nchildren, "calloc(%d,%zu) failed",
                        ALPHABET_SIZE, sizeof(*nchildren));

                struct QsDictionary *n1 = nchildren + (*ee) - START;
                struct QsDictionary *n2 = nchildren + (*cc) - START;
                const char *oldSuffix = node->suffix;
                n1->value = node->value;
                n1->children = node->children;
                n2->value = value;
                 if(node->children == 0) {
        // The root node starts with no children and here we make the
        // root family.
        struct QsDictionary *children =
                calloc(ALPHABET_SIZE, sizeof(*children));
        ASSERT(children, "calloc(%d,%zu) failed",
                ALPHABET_SIZE, sizeof(*children));
        node->children = children;
    }

   node->children = nchildren;
                node->value = 0;

                if(*(ee+1)) {
                    n1->suffix = strdup(ee+1);
                    ASSERT(n1->suffix, "strdup(%p) failed", ee+1);
                }

                ++cc;
                if(*cc) {
                    n2->suffix = strdup(cc);
                    ASSERT(n2->suffix, "strdup(%p) failed", cc);
                }

                if(ee == node->suffix)
                    node->suffix = 0;
                else {
                    *((char *)ee) = '\0';
                    node->suffix = strdup(oldSuffix);
                    ASSERT(node->suffix, "strdup(%p) failed", oldSuffix);
                }

                // We have copied all that we needed from this old suffix.
                free((char *) oldSuffix);

                return 0; // success
            }

            ++c;

#if 0
            // We have no suffix in node
            if(node->value == 0) {
                // And no value, so this is home for this value.
                if(*c) {
                    node->suffix = strdup(c);
                    ASSERT(node->suffix, "strdup(%p) failed", c);
                }
                node->value = value;
                return 0; // success.
            }
#endif
            continue; // See if there are more children.
        }

        // We have no more children
        //
        // Make children, insert and return.
        //
        DASSERT(node->children == 0);

        if(node->value == 0) {
            DASSERT(node->suffix == 0);
            if(*c) {
                // TODO: check c characters are valid.
                node->suffix = strdup(c);
                ASSERT(node->suffix, "strdup(%p) failed", c);
            }
            node->value = value;
            return 0; // success
        }

        struct QsDictionary *children =
            calloc(ALPHABET_SIZE, sizeof(*children));
        ASSERT(children, "calloc(%d,%zu) failed",
                ALPHABET_SIZE, sizeof(*children));
        node->children = children;

        // go to this character (*c) node.
        node = children + (*c) - START;
        node->value = value;
        // add any suffix characters if needed.
        ++c;
        if(*c) {
            // TODO: check c characters are valid.
            node->suffix = strdup(c);
            ASSERT(node->suffix, "strdup(%p) failed", c);
        }
        return 0; // success

    } // for() on characters *c


    // We have no more characters and the last matching node is node.

    if(node->value) {
        ERROR("We have an entry with key=\"%s\"", key);
        return 1;
    }

    // node->value == 0
    node->value = value;
    return 0; // success done
}


// Returns element ptr.
void *qsDictionaryFind(const struct QsDictionary *node, const char *key) {

    for(const char *c = key; *c;) {
        if(node->children) {

            DASSERT(*c >= START);
            DASSERT(*c <= END);
            if(*c < START || *c > END) {
                ERROR("Invalid character in key=\"%s\"", key);
                return 0;
            }
            node = node->children + (*c) - START;
            ++c;

            if(node->suffix) {

                const char *ee = node->suffix;

                for(; *ee && *c && *c == *ee; ++c, ++ee)
                    if(*c < START || *c > END) {
                        ERROR("Invalid character in key=\"%s\"", key);
                        return 0;
                    }

                if(*ee == 0)
                    // Matched so far
                    continue;
                else {
                    // The suffix has more characters that we did not
                    // match.
                    ERROR("No key=\"%s\" found ee=\"%s\"  c=\"%s\"",
                            key, ee, c);
                    return 0;
                }
            }
            continue;
        }

        // No more children, but we have unmatch key characters.
        ERROR("No key=\"%s\" found", key);
        return 0;
    }

    // Hooray!  We got it.
    return (void *) node->value;
}


static void
PrintChildren(const struct QsDictionary *node, char *lastString, FILE *f) {

    if(node->children) {
        struct QsDictionary *end = node->children + ALPHABET_SIZE;
        char c = START;
        for(struct QsDictionary *child = node->children;
                child < end; ++child, ++c) {
            const char *suffix = child->suffix;
            suffix = (suffix?suffix:"");

            if(child->children || child->value) {

                const char *suffix = ((child->suffix)?(child->suffix):"");
                const char *value =
                    ((child->value)?((const char *) child->value):"");

                size_t len = strlen(lastString) + 1 + strlen(suffix) + 1;
                char *str = malloc(len);
                ASSERT(str, "malloc(%zu) failed", len);

                if(child->value)
                    // Create child node id
                    fprintf(f, "  \"%s%c%s\" [label=\"%c%s\\n|%s|\"];\n",
                            lastString, c, suffix, c, suffix, value);
                else
                    // Create child node id
                    fprintf(f, "  \"%s%c%s\" [label=\"%c%s\"];\n",
                            lastString, c, suffix, c, suffix);


                snprintf(str, len, "%s%c%s", lastString, c, suffix);

                PrintChildren(child, str, f);

                free(str);

                fprintf(f, "  \"%s\" -> \"%s%c%s\";\n",
                        lastString, lastString, c, suffix);
            }
        }
    }
}


void qsDictionaryPrintDot(const struct QsDictionary *node, FILE *f) {

    fprintf(f, "digraph {\n  label=\"Trie Dictionary\";\n\n");

    fprintf(f, "  \"\" [label=\"ROOT\"];\n");


    PrintChildren(node, "", f);

    fprintf(f, "}\n");
}
