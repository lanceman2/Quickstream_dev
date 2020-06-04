#include <stdio.h>
#include <inttypes.h>

int main(void) {

    unsigned char x[2];
    uint64_t count = 0;

    while(fread(x, sizeof(x), 1, stdin) == 1)
        printf("%" PRIu64 " %hu %hu\n", count++, x[0], x[1]);

    return 0;
}
