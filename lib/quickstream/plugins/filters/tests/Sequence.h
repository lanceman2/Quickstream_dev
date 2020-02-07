#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "../../../../../lib/debug.h"







// Make a somewhat random ascii character string sequence using
// lrand48_r()

struct RandomString {
    struct drand48_data buffer;
    size_t reminderIndex;
    long int reminder;
    bool init;
};



// len  -- is the length in bytes requested
//
char *randomString_get(struct RandomString *r, size_t len, char *retVal) {

    if(!r->init) {
        // We use the same initialization for all.
        // So the sequence is always the same.
        r->init = true;

        // This is the seed for the random number generator.
        const uint64_t x[3] = {
            0x5421aa1773593c60,
            0x3e13d82d9622b0c3,
            0xf0e05727881623bd
        };

        r->reminderIndex = -1;

        ASSERT(sizeof(r->buffer) == sizeof(x));
        memcpy(&r->buffer, &x, sizeof(r->buffer));
    }

    const char *charSet = "0123456789abcdef";

    char *val = retVal;

    while(len) {

        if(r->reminderIndex !=-1) {
            // Use up the reminder first.
            for(; r->reminderIndex!=-1 && len; --r->reminderIndex) {
                //
                int n = (r->reminder & (15 << (r->reminderIndex*4)))
                    >> (r->reminderIndex*4);
                *val++ = charSet[n]; // one byte at a time.
                --len;
            }
            if(len == 0) break;
        }

        long int result;
        lrand48_r(&r->buffer, &result);

        size_t i=7;
        for(; i!=-1 && len; --i) {
            // Get the next 4 bits as a number, 2^4 = 16
            int n = (result & (15 << i*4)) >> i*4;
            *val++ = charSet[n]; // one byte at a time.
            --len;
        }
        if(len == 0 && i!=-1) {
            r->reminderIndex = i;
            r->reminder = result;
        }
    }

    return retVal;
}



