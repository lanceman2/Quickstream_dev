#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../include/quickstream/qsu.h"
#include "debug.h"



struct QsuRtlsdr *qsu_rtlsdr_open(struct QsuRtlsdr *rtlsdr) {

    const struct QsuUsbdev **devs = qsu_usbdev_find_new(
            "0bda"/*idVender*/,
            "2838" /*idProduct*/ /* for RTL2838UHIDIR  RTL-SDR */,
            "480"/*speed = High Speed - 480Mbits/s */);

    int fd = open(devs[0]->devnode, O_CLOEXEC|O_RDWR);
    qsu_usbdev_find_delete(devs);
    if(fd < 0) {
        ERROR("open(\"%s\",) failed", devs[0]->devnode);
        return 0; // fail.
    }

    rtlsdr->fd = fd;

    return rtlsdr; // success.
}


void qsu_rtlsdr_close(struct QsuRtlsdr *q) {

}
