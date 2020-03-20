#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "../include/quickstream/filter.h"


void catcher(int sig) {
    fprintf(stderr, "Caught signal %u\n"
            "\n"
            "  Try:  gdb -pid %u\n\n"
            "Will now sleep ...\n",
            sig, getpid());
    while(1) {
        usleep(10000);
    }
}



int main(void) {

    signal(SIGSEGV, catcher);

    qsParameterSet(0, 0, 0, 0, 0);

    fprintf(stderr, "SUCCESS\n");
    
    return 0;
}
