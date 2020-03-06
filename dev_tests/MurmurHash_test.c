#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "../lib/debug.h"
#include "../lib/qs.h"

// From: https://github.com/aappleby/smhasher
// MurmurHash1.cpp
extern
uint32_t MurmurHash1(const void *key, int len, uint32_t seed);


int main(void) {

    
    const size_t len_max = 90101;
    const size_t num = 1000;
    size_t len = num;
    uint32_t seed;

    uint64_t dist[len_max];

    for(;len < len_max; ++len) {
        uint64_t i=0;
        for(seed=0xfffff000; seed != 0xfffff400; ++seed) {

            memset(dist, 0, sizeof(dist));
            for(i=0; i<num; ++i) {
                uint64_t key = i + 5;
                uint32_t val = MurmurHash1(&key, sizeof(key), seed);
                size_t bin = val%len;
                dist[val%len] += 1;
                if(dist[bin] > 3 && len != len_max)
                    break;
            }
            if(i == num)
                break;
        }
        if(i == num)
            break;
    }

    for(uint64_t i=0; i<len; ++i)
        printf("%" PRIu64 "\n", dist[i]);

    fprintf(stderr, "Seed=0x%08x  num=%zu len=%zu\n",
            seed, num, len);

    return 0;
}
