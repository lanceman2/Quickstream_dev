#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <inttypes.h>

#include "../include/quickstream/qsu.h"
#include "../lib/debug.h"

// https://www.beyondlogic.org/usbnutshell/usb1.shtml
//
// We want Isochronous Transfer
//
// http://www.hep.by/gnu/kernel/usb/usbfs-ioctl.html
//
// http://www.hep.by/gnu/kernel/usb/usbfs.html
//
// https://www.mail-archive.com/discuss-gnuradio@gnu.org/msg17549.html
//
// https://stackoverflow.com/questions/11574418/how-to-get-usbs-urb-info-in-linux
//
void catchSegv(int sig) {
    fprintf(stderr, "\nCaught signal %d\n"
            "\nsleeping:  gdb -pid %u\n",
            sig, getpid());
    while(true) usleep(100000);
}


int main(void) {

    signal(SIGSEGV, catchSegv);

    struct QsuRtlsdr rtlsdr;
    
    ASSERT(qsu_rtlsdr_open(&rtlsdr));

    qsu_rtlsdr_close(&rtlsdr);

    DSPEW("FINISHED");

    return 0;
}
