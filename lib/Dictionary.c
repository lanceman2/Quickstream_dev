#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "Dictionary.h"
#include "../lib/debug.h"

//  126  ~      is the last nice looking ASCII char
//   32  SPACE  is the first nice looking ASCII char

// We return error on insert if any input character keys are outside this
// range.  We only store the "nice" visible ASCII characters.  This is
// hard enough without thinking about UTF-8.


#define START          (32) // start at SPACE
#define END            (126) // ~

//#define ALPHABET_SIZE  (END + 1 - START) // 95 characters.


// A Dictionary via a Trie
//
// Faster than a hash table, and just a little more memory usage.
// This code ROCKS!
//
// References:
// We use something with less memory than in this:
// https://www.geeksforgeeks.org/trie-insert-and-search/
//
// https://en.wikipedia.org/wiki/Trie
//
// We optimise far more than these references, but they are a nice place
// to start to try to understand this code.
//
//
struct QsDictionary {

    // A Dictionary node.

    // For our use case:
    //   - We do not need fast insert.
    //   - We do not need remove at all.
    //   - Just need fast lookup.
    //   - This will hold about 1000 elements in extreme cases.

    // If there are children than it's an array of length 4.
    //
    // There is no benefit to using a constant variable or CPP MACRO that
    // is 4 because we need to do bit diddling too, that is 3, 2, and 1
    // are constants that are needed too.  The 4 is like a natural
    // constant like Pi in our use case.  Hiding the 4 would just obscure
    // the code.
    //
    // We use a reduced alphabet of size 4 to index this array, otherwise
    // it's 0.  We add a little complexity by having Null children to
    // start with.  We reduce the alphabet to needing 2 bits (2^2=4) as we
    // traverse through the branch points in the tree.
    //
    // Traversing the tree graph example:
    //
    // Trie containing keys "hello" and "heat"
    //
    //     ROOT
    //       |
    //       he____
    //        |    |
    //        llo  at
    //
    //
    // "e" is a branch point that is common to the two keys
    // In ASCII
    //   'e' = oct 0145 = hex 0x65 = dec 101 = binary 01100101 = 01 10 01 01
    //   'l' = oct 0154 = hex 0x6C = dec 108 = binary 01101100 = 01 10 11 00
    //   'a' = oct 0141 = hex 0x61 = dec  97 = binary 01100001 = 01 10 00 01
    //
    // "hello" deviates from "heat" at the 'l' and the 'a':
    // and so the deviation is at the first and second bits being
    //  00 ('l') and 01 ('a') when we fix the 2 bit left to right ordering,
    // so the hello traversal reads like 'h' 'e' 00 11 10 01 'l' 'o'
    // or                                'h' 'e' 0  3  2  1  'l' 'o'
    // adding 1 to the 2 bit values =>   'h' 'e' 1  4  3  2  'l' 'o'
    // and since ASCII decimal 1 2 3 and 4 are invalid characters in our
    // use we can use them is encode the missing 'l' as the 8 bit byte
    // sequence 1 4 3 2.  We can't use 0 because that would terminate our
    // string, so we are using 'l' = (c0-1) << 0 + (c1-1) << 2 + (c2-1) << 4
    // + (c3-1) << 6 where c0 = ASCII byte 1, c1 = ASCII byte 4,
    // c2 = ASCII byte 3, and c3 = ASCII byte 2.
    //
    // Doing the same analysis for "heat" gives
    //
    // "heat" traversal is   'h' 'e' 1 0 2 1 't'
    // adding the 1       => 'h' 'e' 2 1 3 2 't'
    //
    // At the cost of converting characters after branch points we can use
    // a lot less memory.  Each branch point can only have a 1 2 3 or 4
    // as a starting character.
    //
    // The example tree looks like:
    //
    //     ROOT
    //       |
    //       he_______________
    //        |               |
    //       \1\4\3\2lo      \2\1\3\2t
    //
    //
    // where we define \1 as the bit sequence 01 that we convert to 00,
    // and so on.
    //
    //
    // Adding key = "heep" gives ==> he\2\2\3\2p
    //
    //
    //     ROOT
    //       |
    //       he_________________
    //        |                 |
    //        |                 |
    //       \1\4\3\2lo         \2___________________
    //                           |                    |
    //                           |                    |
    //                          \1\3\2t              \2\3\2p
    //
    //
    // where \1 \2 \3 \4 are just one byte characters that are 1 2 3 4.
    // We will store them as bytes even though they only contain 2 bits
    // of information, saving computation time at the cost of using a
    // little more memory at branch points.
    //
    // But wait there's more.  The top branch point is from the "he" where
    // e is shared below.  But we can't use 'e' are a branch point, this
    // 'e' must be made into a 2 bit group like so:  'e' = \2\2\3\2  so:
    //
    //
    //
    //     ROOT
    //       |
    //       h\2\2\3\2__________________
    //               |                  |
    //               |                  |
    //              \1\4\3\2lo         \2____________________
    //                                  |                    |
    //                                  |                    |
    //                                 \1\3\2t              \2\3\2p
    //
    //
    // But wait there's even move.  The 'h' can't be a branch point from
    // root so  'h' = ASCII 150 = binary 01101000 = 01 10 10 00
    // = \2 \3 \3 \1 = \1\3\3\2 in left to right order so:
    //
    //     ROOT
    //       |
    //      \1\3\3\2\2\2\3\2__________________
    //                     |                  |
    //                     |                  |
    //                    \1\4\3\2lo         \2____________________
    //                                        |                    |
    //                                        |                    |
    //                                       \1\3\2t              \2\3\2p
    //
    //
    //
    // Clearly with such short keys there is more stuff like \1\2\3\4 than
    // there would be had they been longer keys.
    //
    //
    //
    struct QsDictionary *children; // array of 4 when needed and allocated

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
    char *suffix;

    // value stored if there is one.  The key for this value is the string
    // generated by traversing the graph stringing together the characters
    // and optional suffix (if present) at each node in the traversal.
    // Sometimes this node is just a branch point with no value.
    const void *value;
};


struct QsDictionary *qsDictionaryCreate(void) {

    struct QsDictionary *d = calloc(1, sizeof(*d));
    ASSERT(d, "calloc(1,%zu) failed", sizeof(*d));

    struct QsDictionary *children = calloc(4, sizeof(*children));
    ASSERT(children, "calloc(%d,%zu) failed", 4, sizeof(*children));
    d->children = children;

    return d;
}


static
void FreeChildren(struct QsDictionary *children) {

    for(struct QsDictionary *i = children + 4 - 1;
            i >= children; --i)
        if(i->children)
            // Recurse
            FreeChildren(i->children);

    if(children->suffix)
        free(children->suffix);

#if DEBUG
    memset(children, 0, 4*sizeof(*children));
#endif
    free(children);
}


void qsDictionaryDestroy(struct QsDictionary *dict) {

    DASSERT(dict);

    if(dict->children)
        FreeChildren(dict->children);

    if(dict->suffix)
        free(dict->suffix);

#if DEBUG
    memset(dict, 0, sizeof(*dict));
#endif
    free(dict);
}


// TODO: Remove this debugging print thing.
static inline char *
STRING(const char *s) {

    static char *str = 0;

    str = realloc(str, strlen(s) * 4 + 2);
    ASSERT(str, "realloc() failed");

    char *ss = str;

    for(; *s; ++s) {
        if(*s >= START)
            *ss++ = *s;
        else {
            *ss++ = '\\';
            *ss++ = *s + '0';
        }
    }
    *ss = '\0';

    return str;
}


// Example:
//
// Input: "key"
//             k        e       y
// Output: \4\3\3\2 \2\2\3\2 \2\3\4\2
//
// It's a 4 x 2 bit encoding of each character.  0 is not used to encode
// because we still need a 0 (Null) terminator to be 0 and not part of the
// encoding.   So the 2 bits are 00 01 10 11 and than we add one giving
// 01 10 11 100 = 1 2 3 4 = \1 \2 \3 \4  so we can tell them from '1' '2'
// '3' and '4'.  See above.
//
// Returns allocated string.
static inline
char *Expand(const char *key) {

    size_t l = 1;
    for(const char *c = key; *c; ++c) {
        if(*c > 4)
            l += 4;
        else
            ++l;
    }

    char *s = malloc(l);
    ASSERT(s, "malloc(%zu) failed", l);
    char *str = s;

    for(const char *c = key; *c; ++c) {

        uint32_t code = *c;

        if(code > 4) {
            uint32_t *val = (uint32_t *) str;

            *val =     (((0x00000003) & code)        + 0x00000001)
                    | ((((0x0000000C) & code) << 6 ) + 0x00000100)
                    | ((((0x00000030) & code) << 12) + 0x00010000)
                    | ((((0x000000C0) & code) << 18) + 0x01000000);
            str += 4;
        } else
            *(str++) = code;
    }

    *str = '\0';

    return s;
}

static inline
bool
IsEndOfCharacter(const char *suffix, const char *end, uint32_t count) {

    DASSERT((end-3) >= suffix);
    DASSERT(*end && *(end-1) && *(end-2) && *(end-3));

    return false;

    if(
            count%4 == 0 &&
            *end < 5 &&
            *(end-1) < 5 &&
            *(end-2) < 5 &&
            *(end-3) < 5
            )
        return true;
    return false;
}


// Returns allocated string.
//
// Comverts in parts of 4 characters from the end.
//
// Examples:
//
// Input: \3\2 \2\2\3\2 \2\3\4\2
//
// Output: \3\2ey
//
static inline
char *Compress(const char *suffix, uint32_t count) {

    DASSERT(suffix);
    DASSERT(*suffix);

    size_t l = strlen(suffix);
    size_t newL = l;

    const char *end = suffix + l - 1;

    uint32_t i = count + l - 1;

    while(end-3 >= suffix) {
        if(IsEndOfCharacter(suffix, end, i)) {
            newL -= 3;
            end -= 4;
            i -= 4;
        } else {
            --end;
            --i;
        }
    }

    char *new = malloc(newL + 1);
    ASSERT(new, "malloc(%zu) failed", newL + 1);
    char *newEnd = new + newL - 1;
    new[newL] = '\0';
    end = suffix + l - 1;
    i = count + l - 1;

    while(end-3 >= suffix) {
        if(IsEndOfCharacter(suffix, end, i)) {
            char val = (*(end--) - 1) << 6;
            val     |= (*(end--) - 1) << 4;
            val     |= (*(end--) - 1) << 2;
            val     |= (*(end--) - 1);
            *newEnd-- = val;
            i -= 4;
        } else {
            *newEnd-- = *end--;
            --i;
        }
    }

    // Finish it filling in first few characters.
    while(end >= suffix)
        *newEnd-- = *end--;


DSPEW("count=%u \"%s\"", count, STRING(new));

    return new;
}



// Speed of Insert is not much of a concern.  It's Find that needs to be
// fast.
//
// Returns 0 on success or 1 if already present and -1 if it is not added
// and have an invalid character.
int qsDictionaryInsert(struct QsDictionary *node,
        const char *key_in, const void *value) {

    DASSERT(node);
    DASSERT(key_in);
    DASSERT(*key_in);
    DASSERT(value);

    // Since we have no need for speed in Insert and the code is simpler
    // this way, we check all the characters in the key at the start of
    // this function.  It would have be faster (but more complex) to do
    // this as we traverse the characters in the key in the next for loop.
    //
    for(const char *c = key_in; *c; ++c)
        if(*c < START || *c > END) {
            ERROR("Invalid character in key: \"%s\"",
                    key_in);
            return -1;
        }

    // We put the key input the form like: "he" = \1\3\3\2 \2\2\3\2
    char *key = Expand(key_in);

#if 0
    for(char *str = key; *str; ++str) {
        fprintf(stderr, "\\%d", *str);
    }
    fprintf(stderr, "\n");

    return 0;
#endif

    // It turns out we also need an index character counter in order to
    // tell when we can compress the likes of \2\2\3\2 to 'h'.  That can
    // only happen at every 4 characters starting at the top of the tree.
    // If it compresses across a "boundary" we get trouble decoding it.
    size_t count = 0;

    for(char *c = key; *c;) {

        if(node->children) {
            // We will go to next child in the traversal.
            //
            // Go to the next child in the traversal.

            node = node->children + *c - 1;
            ++c;
            ++count;

            if(node->suffix) {

                char *eSuffix = Expand(node->suffix); // gets freed.
                char *e = eSuffix;

                for(;*e && *c && *e == *c; ++c, ++e, ++count);
                // e and c are at:
                //
                // CASES:
                //
                //
                if(*e == 0 && *c == 0) {
                    // Prefect match.
                    if(node->value) {
                        ERROR("Entry for key=\"%s\" exists", key_in);
                        free(eSuffix);
                        free(key);
                        return 1;
                    }
                    node->value = value;
                    free(eSuffix);
                    free(key);
                    return 0;
                }
                if(*e == 0) {
                    // CONTINUE
                    //
                    // Suffix matched and ran out.  Still have key
                    // characters.  Go to the next child if there is one.
                    free(eSuffix);
                    continue;
                }
                if(*c == 0) {
                    // SPLIT
                    //
                    // The key ran out and the suffix still have
                    // characters.
                     // We matched part way through the suffix.
                    //
                    // So: split the node in two and the first one has the
                    // new inserted value and the second (n2) has the old
                    // value and the old children.
                    //
                    // New node children:
                    struct QsDictionary *children = calloc(4,
                            sizeof(*children));
                    ASSERT(children, "calloc(%d,%zu) failed",
                            4, sizeof(*children));

                    // The first node keeps the same child memory as node.
                    struct QsDictionary *n2 = children + (*e) - 1;
                    n2->children = node->children;
                    n2->value = node->value;

                    if(*(e+1))
                        // Copy the remains of the old suffix starting at
                        // the next character.
                        n2->suffix = Compress(e+1, count+1);

                    char *oldSuffix = node->suffix;
                    node->value = value;
                    if(e == eSuffix) {
                        // There where no matching chars in suffix and the
                        // char pointers never advanced.
                        node->suffix = 0;
                    } else {
                        // At least one char matched in suffix.  Null
                        // terminate the suffix after the last matching
                        // char and then dup it.
                        *(char *) e = '\0'; // It's okay we copied it
                        // above.  We have less characters for this node.
                        node->suffix = Compress(eSuffix, count - (e - eSuffix));
                    }
                    node->children = children;

                    free(oldSuffix);
                    free(eSuffix);
                    free(key);
                    return 0; // success
                }

                // BIFURCATE
                //
                // We match up to "ee" but there is more to go and we
                // don't match after that.  Result is we need to add 2
                // nodes.

                // New node children:
                //
                // 2 of these children will be used in this bifurcation.
                struct QsDictionary *children =
                        calloc(4, sizeof(*children));
                ASSERT(children, "calloc(%d,%zu) failed",
                        4, sizeof(*children));

                char *oldSuffix = node->suffix;
                struct QsDictionary *n1 = children + (*e) - 1;
                struct QsDictionary *n2 = children + (*c) - 1;
                n1->value = node->value;
                n1->children = node->children;
                n2->value = value;
                node->children = children;
                node->value = 0;

                if(*(e+1))
                    n1->suffix = Compress(e+1, count+1);

                ++c;
                if(*c)
                    n2->suffix = Compress(c, count+1);

                if(e == eSuffix)
                    node->suffix = 0;
                else {
                    // No need to worry, we copied this above, so now we
                    // came hack it up.
                    *((char *)(e)) = '\0';
                    node->suffix = Compress(eSuffix, count - (e - eSuffix));
                }

                free(oldSuffix);
                free(eSuffix);
                free(key);
                return 0; // success

            } // if(node->suffix)

            // The simple case.  We matched, to get to this node.
            // We'll act after we break out of this 
            continue; // See if there are more children.

        } // if(node->children) { // node = next child


        DASSERT(node->children == 0);
        DASSERT(*c);

        if(node->value == 0) {
            DASSERT(node->suffix == 0);
            node->suffix = Compress(c, count);
            node->value = value;

            free(key);
            return 0; // success
        }

        struct QsDictionary *children = calloc(4, sizeof(*children));
        ASSERT(children, "calloc(%d,%zu) failed", 4, sizeof(*children));
        node->children = children;

        // go to this character (*c) node.
        node = children + (*c) - 1;
        node->value = value;
        // add any suffix characters if needed.
        if(*(c+1))
            node->suffix = Compress(c+1, count+1);

        free(key);
        return 0; // success


    } // for(char *c = key; *c; ++c) {

    if(node->value) {
        ERROR("We have an entry with key=\"%s\"", key);
        free(key);
        return 1;
    }

    // node->value == 0
    node->value = value;
    free(key);
    return 0; // success done
}



// Returns the next character
// This returns the next character after we run out of the 2 bit encoded
// characters.
static inline
char GetChar(char **bits, const char **c) {

    char ret;

    if(**bits) {
        // For this use case we encode the 0 as 1 and the 1 as 2
        // and the 2 as 3 and the 3 as 4; because we need 0 to terminate
        // the string.
        DASSERT((**bits) <= 4);
        ret = (**bits);
        *bits += 1;
        if(**bits == 0)
            // We hit the 0 terminator.
            // Advance to the next character for next time.
            *c += 1;
        // Return a 2 bits of a byte + 1.
        return ret;
    }

    // *bits is at the null terminator.
    ret = **c;

    *c += 1;

    return ret;
}


// I wish C had pass by reference.  This is only used to index into a
// child node.  So the 1 offset is not added.
static inline
char GetBits(char **bits, const char **c) {

    // We cannot call this function if there are no more characters left
    // in the string.
    DASSERT(**c);

    char ret;

    if(**bits) {
        // For this use case we encode the 0 as 1 and the 1 as 2
        // and the 2 as 3 and the 3 as 4; because we need 0 to terminate
        // the string.
        DASSERT((**bits) <= 4);
        ret = (**bits);
        *bits += 1;
        if(**bits == 0)
            // We hit the 0 terminator.
            // Advance to the next character for next time.
            *c += 1;
        // Return a 2 bits of a byte + 1.
        return ret;
    }

    // *bits is at the null terminator.

    // The current character we analyze is:
    ret = **c;

    // Bit diddle with next character, i.e. decompose it into 4 2 bit
    // pieces in bits[].  We store 2 bits in each char in bits[4].  The
    // value of any one of bits[] is 0, 1, 2, or 3.  We encode 1 8 bit
    // char as 4 8 byte chars with values only 0, 1, 2, or 3.

    // Go to the second array element in this null terminate array of 4 +
    // 1 null = *bits[5].
    //
    *bits -= 3;

    // We can use a pointer to a 32 bit = 4 byte x 8 bit/byte thing to set
    // all the *bits[] array at one shot.
    uint32_t *val32 = (uint32_t *) (*bits - 1);

    // We must keep this order when decoding in the Find() function.

    *val32 =   (((0x00000003) & ret)        + 0x00000001)
            | ((((0x0000000C) & ret) << 6)  + 0x00000100)
            | ((((0x00000030) & ret) << 12) + 0x00010000)
            | ((((0x000000C0) & ret) << 18) + 0x01000000);
    //
    // The next bits[] to be returned after this call will be **bits =
    // *bits[0], not *bits[-1].
    //
    return *(*bits -1); // Return the first set of 2 bits.
}


// Returns element ptr.
void *qsDictionaryFind(const struct QsDictionary *node, const char *key) {

    DASSERT(key);
    DASSERT(key[0]);

    // Setup pointers to an array of chars that null terminates.
    char b[5];
    char *bits = b + 4;
    b[0] = 1;
    b[1] = 1;
    b[2] = 1;
    b[3] = 1;
    b[4] = 0; // null terminate.
    // We'll use bits to swim through this "bit" character array.

    for(const char *c = key; *c; ) {

        if(node->children) {

            node = node->children + GetBits(&bits, &c)  - 1;

            if(node->suffix) {

                const char *e = node->suffix;

                // Find the point where key and suffix do not match.
                for(;*e && *c;) {
                    // Consider iterating over 2 bit chars
                    if(*e < START) {
                        if((*e) != GetBits(&bits, &c))
                            break;
                    } else
                    // Else iterate over regular characters.
                    if(*e != GetChar(&bits, &c))
                        break;
                    ++e;
                }

                if(*e == 0)
                    // Matched so far
                    continue;
                else {
                    // The suffix has more characters that we did not
                    // match.
                    ERROR("No key=\"%s\" found", key);
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
PrintEscChar(char c, FILE *f) {

    DASSERT((1 <= c && c <= 4) ||
            (START <= c && c <= END), "c=\\%d", c);

    if(c < '0' || ('9' < c && c < 'A') ||
            ('Z' < c && c < 'a') || 'z' < c)
        fprintf(f, "\\\\%d", c); // like \\2
    else
        // Print like a regular character like 'a' or '5'.
        putc(c, f);
}


static void
PrintEscStr(const char *s, FILE *f) {

    while(*s)
        PrintEscChar(*s++, f);
}


static void
Print_Char(char c, FILE *f) {

    DASSERT((1 <= c && c <= 4) ||
            (START <= c && c <= END), "c=\\%d", c);

    if(c < '0' || ('9' < c && c < 'A') ||
            ('Z' < c && c < 'a') || 'z' < c)
        fprintf(f, "_%d", c); // like \\2
    else
        // Print like a regular character like 'a' or '5'.
        putc(c, f);
}


static void
Print_Str(const char *s, FILE *f) {

    while(*s)
        Print_Char(*s++, f);
}

#if 1
static void
PrintStr(const char *s, FILE *f) {
    PrintEscStr(s,f);
}
#else
static void
PrintStr(const char *s, FILE *f) {


    while(*s) {


        if(*s < 5) {

            DASSERT(*(s+1) && *(s+2) && *(s+3));
            DASSERT(*(s+1) < 5, "*(s+1)=%d", *(s+1));
            DASSERT(*(s+2) < 5, "*(s+2)=%d", *(s+2));
            DASSERT(*(s+3) < 5, "*(s+3)=%d", *(s+3));

            char val = (*s - 1);
            ++s;
            val |= (*s - 1) << 2;
            ++s;
            val |= (*s - 1) << 4;
            ++s;
            val |= (*s - 1) << 6;

            PrintEscChar(val, f);
        } else

            PrintEscChar(*s, f);
        ++s;
    }
}
#endif


// This function is called recursively adding to parentPrefix at each
// level in the call stack.
static void
PrintChildren(const struct QsDictionary *node, char *parentPrefix,
        char *parentSum, FILE *f) {

    if(node->children) {
        char c = 1;
        for(struct QsDictionary *child = node->children;
                c <= 4; ++c, child = node->children + c - 1) {

            if(child->value == 0 && child->children == 0) continue;

            const char *suffix = child->suffix;

            // Make a new longer prefix
            size_t l1 = strlen(parentPrefix);
            char *prefix = malloc(l1 + 2);
            ASSERT(prefix, "malloc() failed");
            strcpy(prefix, parentPrefix);
            *(prefix + l1) = c;
            *(prefix + l1 + 1) = '\0';

            // Make a longer sum
            l1 = strlen(parentSum);
            size_t l2 = strlen(suffix?suffix:"");
            size_t len = l1 + 1 + l2 + 1;
            char *sum = malloc(len);
            ASSERT(sum, "malloc() failed");
            strcpy(sum, parentSum);
            *(sum + l1) = c;
            strcpy(sum + l1 + 1, suffix?suffix:"");
            *(sum + l1 + 1 + l2) = '\0';

            // parent -> child
            fprintf(f, "  \"");
            Print_Str(parentPrefix, f);
            fprintf(f, "\" -> \"");
            Print_Str(prefix, f);
            fprintf(f, "\";\n");

            // child label
            fprintf(f, "  \"");
            Print_Str(prefix, f);
            fprintf(f, "\" [label=\"");
            PrintEscChar(c, f);
            if(suffix) {
                fprintf(f, "\\nsuffix=|");
                PrintEscStr(suffix, f);
                fprintf(f, "|");
            }
            fprintf(f, "\\ntotal=");
            PrintEscStr(sum, f);
            fprintf(f, "");

            if(child->value) {

                fprintf(f, "\\nkey=\\{ ");
                PrintStr(sum, f);
                fprintf(f, " \\}");
            }

            fprintf(f, "\"];\n");

            PrintChildren(child, prefix, sum, f);
            
            free(prefix);
            free(sum);
        }
    }
}


void qsDictionaryPrintDot(const struct QsDictionary *node, FILE *f) {

    fprintf(f, "digraph {\n  label=\"Trie Dictionary\";\n\n");

    fprintf(f, "  \"\" [label=\"ROOT\"];\n");


    PrintChildren(node, "", "", f);

    fprintf(f, "}\n");
}
