#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdbool.h>
#include <linux/usbdevice_fs.h>
#include <linux/usb/ch9.h>


#include "../include/quickstream/qsu.h"
#include "qsu.h"
#include "debug.h"


struct QsuRtlsdr *qsu_rtlsdr_new(void) {

    const struct QsuUsbdev **devs = qsu_usbdev_find_new(
            "0bda"/*idVender*/,
            "2838" /*idProduct*/ /* for RTL2838UHIDIR  RTL-SDR */,
            "480"/*speed = High Speed - 480Mbits/s */);

    if(!devs) {
        ERROR("No known UBS devices found");
        return 0;
    }

    const char *path = devs[0]->path;
    DASSERT(path);

    struct QsuRtlsdr *rtl = calloc(1, sizeof(*rtl));
    ASSERT(rtl, "alloc(1,%zu) failed", sizeof(*rtl));
    rtl->fd = -1;

    if(_qsu_rtlsdr_GetDescriptor(&rtl->descriptor, path)) {
        qsu_rtlsdr_delete(rtl);
        return 0;
    }

    int fd = open(devs[0]->devnode, O_CLOEXEC|O_RDWR);
    qsu_usbdev_find_delete(devs);
    if(fd < 0) {
        ERROR("open(\"%s\",) failed", devs[0]->devnode);
        qsu_rtlsdr_delete(rtl);
        return 0; // fail.
    }

#if 0
    struct usbdevfs_urb urb;


#endif

    rtl->fd = fd;

    return rtl; // success.
}


void qsu_rtlsdr_delete(struct QsuRtlsdr *rtl) {

    if(rtl == 0) return;

    if(rtl->fd >= 0)
        close(rtl->fd);

    struct Descriptor *d = &rtl->descriptor;

    if(d->device.bNumConfigurations) {

        for(uint8_t i = d->device.bNumConfigurations - 1;
                i != (uint8_t) -1; --i)
            _qsu_rtlsdr_FreeConfig(d->configs + i);

#ifdef DEBUG
        memset(d->configs, 0,
                d->device.bNumConfigurations*sizeof(*d->configs));
#endif
        free(d->configs);
    }

#ifdef DEBUG
    memset(rtl, 0, sizeof(*rtl));
#endif

    free(rtl);
}
