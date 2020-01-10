#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>


#include "../lib/debug.h"


static void catcher(int signum) {

    WARN("Caught signal %d\n", signum);
    fprintf(stderr, "Try running:\n\n"
            "    gdb -pid %u\n\n", getpid());
    while(1) usleep(10000);
}



int main(void) {

    signal(SIGSEGV, catcher);


    DASSERT(0);

    return 0;

}
