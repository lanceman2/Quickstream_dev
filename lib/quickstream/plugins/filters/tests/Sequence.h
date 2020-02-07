#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "../../../../../lib/debug.h"


// What we want:
//
// 1. pseudo-random data.
// 2. ASCII format so we can look at it.
// 3. Generated on the fly and not from a file.
// 4. Does not have to be high quality.
// 5. Works with the program diff so adding '\n' is nice.
//

//////////////////////////////////////////////////////////////////////
//                 CONFIGURATION
//////////////////////////////////////////////////////////////////////
#define NEWLINE_LEN (50)

// Got seeds from running: sha512sum ANYFILE.
#define SEED_0 (0x5421aa1773593c60)
#define SEED_1 (0x3e13d82d9622b0c3)
#define SEED_2 (0xf0e05727881623bd)
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////



// Make a somewhat random ascii character string sequence using
// lrand48_r()

struct RandomString {
    struct drand48_data buffer;
    long int remainder;
    int remainderIndex;
    int newlineLen;
    int newlineIndex;
    bool init;
};


// Returns the remaining length.
static inline 
size_t GetNextStrings(struct RandomString *r,
        int *index, long int field, char **val_in, size_t len) {

    const char *charSet = "0123456789abcdef";
    int i = *index;
    char *val = *val_in;

    while(i!=-1 && len) {

        ++r->newlineIndex;

        if(r->newlineLen && r->newlineIndex == r->newlineLen) {
            *val++ = '\n';
            --len;
            r->newlineIndex = 0;
            continue;
        }

        // Get the next 4 bits as a number, 2^4 = 16
        int n = (field & (15 << i*4)) >> i*4;
        *val++ = charSet[n]; // one byte at a time.
        --len;
        --i;
    }

    *val_in = val;
    *index = i;
    return len;
}


// len  -- is the length in bytes requested
//
// The sequence generated is not dependent on len.
//
static inline
char *randomString_get(struct RandomString *r, size_t len, char *retVal) {

    if(!r->init) {
        // We use the same initialization for all.
        // So the sequence is always the same.
        r->init = true;

        // This is the seed for the random number generator.  Change this
        // to change the sequence.  Got it from the program sha512sum.
        const uint64_t x[3] = {
            0x5421aa1773593c60,
            0x3e13d82d9622b0c3,
            0xf0e05727881623bd
        };

        r->remainderIndex = -1;

        ASSERT(sizeof(r->buffer) == sizeof(x));
        memcpy(&r->buffer, &x, sizeof(r->buffer));
        r->newlineLen = NEWLINE_LEN;
        r->newlineIndex = 0;
    }

    char *val = retVal;

    while(len) {

        if(r->remainderIndex !=-1) {
            // Use up the reminder first.
            len = GetNextStrings(r, &r->remainderIndex, r->remainder, &val, len);
            if(len == 0) break;
        }

        long int result;
        lrand48_r(&r->buffer, &result);

        int i=7;
        len = GetNextStrings(r, &i, result, &val, len);

        if(len == 0 && i!=-1) {
            r->remainderIndex = i;
            r->remainder = result;
        }
    }

    return retVal;
}
