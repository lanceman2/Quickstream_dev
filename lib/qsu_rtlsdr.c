#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <fcntl.h>
#include <errno.h>
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

static bool ClaimInterface(struct QsuRtlsdr *rtl) {
   
    unsigned int i = 1;
    errno = 0;
    int r = ioctl(rtl->fd, USBDEVFS_CLAIMINTERFACE, &i);
    DSPEW("ioctl(%d, USBDEVFS_CLAIMINTERFACE,%p) = %d  errno=%d",
            rtl->fd, &i, r, errno);
    return (r == 0)?false:true; // true for error
}


static bool
Get(struct QsuRtlsdr *rtl) {
   
    errno = 0;


    struct usbdevfs_ioctl ctl;
    ctl.ifno = 0;
    ctl.ioctl_code = USBDEVFS_DISCONNECT;
    ctl.data = 0;

    int r = ioctl(rtl->fd, USBDEVFS_IOCTL, &ctl);
    DSPEW("ioctl(, USBDEVFS_IOCTL,) = %d", r);



    struct usbdevfs_urb *suburb = malloc(sizeof(*suburb));
    ASSERT(suburb);
    struct usbdevfs_urb *returb = 0;
    const int len = 1024;
    char *buffer = malloc(len+1);

    memset(suburb, 0, sizeof(*suburb));

    suburb->usercontext = "hello";
    suburb->type = USBDEVFS_URB_TYPE_CONTROL;
    suburb->endpoint = 0;
    suburb->buffer = buffer;
    suburb->buffer_length = len;
    errno = 0;
    r = ioctl(rtl->fd, USBDEVFS_SUBMITURB, suburb);
    DSPEW("ioctl(%d, USBDEVFS_SUBMITURB,%p) = %d  errno=%d",
            rtl->fd, suburb, r, errno);

    r = ioctl(rtl->fd, USBDEVFS_REAPURB, &returb);
    DSPEW("ioctl(%d, USBDEVFS_REAPURB,%p) = %d  errno=%d returb=%p",
            rtl->fd, &returb, r, errno, returb);

    buffer[len] = '\0';

    DSPEW("returb->buffer=\"%s\"", (char *) returb->buffer);

    ASSERT(0);


    return false;
}




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

    // This seems to get set above.
    errno = 0;
   
    int fd = openat(AT_FDCWD, devs[0]->devnode, O_CLOEXEC|O_RDWR);
    qsu_usbdev_find_delete(devs);
    if(fd < 0) {
        ERROR("open(\"%s\",) failed", devs[0]->devnode);
        qsu_rtlsdr_delete(rtl);
        return 0; // fail.
    }

    rtl->fd = fd;



    {
        uint32_t caps = 0;
        int r = ioctl(fd, USBDEVFS_GET_CAPABILITIES, &caps);
        ASSERT(r == 0, "ioctl(%d, USBDEVFS_GET_CAPABILITIES,) failed", fd);
        DSPEW("caps=0%o = 0x%x", caps, caps);
    }


#ifdef SPEW_LEVEL_NOTICE
    {
        struct usbdevfs_getdriver getdriver;

        int r = ioctl(fd, USBDEVFS_GETDRIVER, &getdriver);
        ASSERT(r == 0);
        NOTICE("Using driver: %s", getdriver.driver);
    }
#endif

    ClaimInterface(rtl);

    Get(rtl);


    

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
