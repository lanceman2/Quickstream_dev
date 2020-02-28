#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>

#include "../include/quickstream/qsu.h"
#include "../lib/debug.h"

// https://www.beyondlogic.org/usbnutshell/usb1.shtml

void catchSegv(int sig) {
    fprintf(stderr, "\nCaught signal %d\n"
            "\nsleeping:  gdb -pid %u\n",
            sig, getpid());
    while(true) usleep(100000);
}


int main(void) {

    signal(SIGSEGV, catchSegv);

    const char **devices = qsu_usbdev_find_new("0bda"/*idVender*/,
            "2838" /*idProduct*/ /* for RTL2838UHIDIR  RTL-SDR */,
            "480"/*speed = High Speed - 480Mbits/s */);

    for(const char **d = devices; d && *d; ++d)
        printf("found %s\n", *d);

    if(!devices) {
        WARN("No usable devices found");
        return 1;
    }

    errno = 0;
    int fd = open(devices[0], O_CLOEXEC|O_RDWR);
    ASSERT(fd >= 0, "open(\"%s\",0) failed", devices[0]);

    qsu_usbdev_find_delete(devices);

    // linux/usbdevice_fs.h
    // #define USBDEVFS_GET_CAPABILITIES  _IOR('U', 26, __u32)
    //
    // linux_usbfs.c
    // IOCTL_USBFS_GET_CAPABILITIES

    uint32_t caps = 0;

    int r = ioctl(fd, USBDEVFS_GET_CAPABILITIES, &caps);

    ASSERT(r == 0, "ioctl(%d, USBDEVFS_GET_CAPABILITIES,) failed", fd);


    DSPEW("caps=%" PRIu32, caps);






    const char *msg = "exiting SUCCESS\n";

    // We added a know system call for our running:
    // 'strace ./udev_test'
    //
    write(2, msg, strlen(msg));

    return 0;
}
