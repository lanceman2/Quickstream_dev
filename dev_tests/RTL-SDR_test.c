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
#include <stdbool.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>

#include "../include/quickstream/qsu.h"
#include "../lib/debug.h"

// https://www.beyondlogic.org/usbnutshell/usb1.shtml
//
// We want Isochronous Transfer

void catchSegv(int sig) {
    fprintf(stderr, "\nCaught signal %d\n"
            "\nsleeping:  gdb -pid %u\n",
            sig, getpid());
    while(true) usleep(100000);
}




int main(void) {

    signal(SIGSEGV, catchSegv);

    const struct QsuUsbdev **devs =
        qsu_usbdev_find_new("0bda"/*idVender*/,
            "2838" /*idProduct*/ /* for RTL2838UHIDIR  RTL-SDR */,
            "480"/*speed = High Speed - 480Mbits/s */);

    for(const struct QsuUsbdev **d = devs; d && *d; ++d)
        fprintf(stderr, "found %s\n", (*d)->devnode);

    if(!devs) {
        WARN("No usable devices found");
        return 1;
    }

    qsu_usbdev_descriptor_print_dot(stdout, devs[0]->path);

    int fd = open(devs[0]->devnode, O_CLOEXEC|O_RDWR);
    ASSERT(fd >= 0, "open(\"%s\",0) failed", devs[0]->devnode);

    qsu_usbdev_find_delete(devs);

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
