#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>

#include "../include/quickstream/qsu.h"

void catchSegv(int sig) {
    fprintf(stderr, "\nCaught signal %d\n"
            "\nsleeping:  gdb -pid %u\n",
            sig, getpid());
    while(true) usleep(100000);
}

// USB isochronous transfer urb  MAX_ISO_PACKETS_PER_URB
// https://www.kernel.org/doc/html/latest/driver-api/usb/URB.html
// https://stackoverflow.com/questions/19593420/isochronous-usb-transfers-confusion#19624959
// https://www.linuxjournal.com/article/8093
// https://www.beyondlogic.org/usbnutshell/usb4.shtml#Isochronous
// https://www.beyondlogic.org/usbnutshell/usb1.shtml

int main(void) {

    signal(SIGSEGV, catchSegv);

    const char **devices = qsu_usbdev_new("0bda"/*idVender*/,
            "2838" /*idProduct*/ /* for Generic RTL2832U OEM */);

    qsu_usbdev_delete(devices);

    const char *msg = "exiting\n";

    write(2, msg, strlen(msg));

    return 0;
}
