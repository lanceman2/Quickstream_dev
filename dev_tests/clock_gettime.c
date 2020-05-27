#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>


int main(void) {

    int i;
    //clockid_t type = CLOCK_MONOTONIC_COARSE;
    clockid_t type = CLOCK_MONOTONIC;
    struct timespec start, end;
    clock_gettime(type, &start);

    double total = 0;

    for(i=0; i<10000; ++i) {
        //usleep(5000);
        clock_gettime(type, &end);
        double val = (end.tv_sec - start.tv_sec) + 1.0e-9 * (end.tv_nsec - start.tv_nsec);
        total += val;
        printf("%d %.15lg\n", i, val);
        memcpy(&start, &end, sizeof(start));
    }
    printf("total=%.15lg\n", total);
    return 0;
}
