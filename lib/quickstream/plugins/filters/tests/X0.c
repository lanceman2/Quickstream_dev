#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include "Sequence.h"


int main(void) {

    struct RandomString r;
    memset(&r, 0, sizeof(r));

    const size_t LEN = 1000;
    char str[LEN];

    size_t totalBytes = 100000;

    size_t currentBytes = 0;

    while(currentBytes < totalBytes) {

        size_t lenOut = LEN-1;
        if(currentBytes + lenOut > totalBytes)
            lenOut = totalBytes - currentBytes;

        char *out = randomString_get(&r, lenOut, str);
        out[lenOut] = '\0';

        printf("%s", out);
        currentBytes += lenOut;
    }

    printf("\n");

    fprintf(stderr, "done\n");

    return 0;
}
