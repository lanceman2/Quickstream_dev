#include <stdio.h>
#include <libudev.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>

#include "debug.h"
#include "../include/quickstream/qsu.h"


const char **
qsu_usbdev_new(const char *idVender, const char *idProduct) {

    struct udev *udev = udev_new();
    const char **ret = 0;
    uint32_t n = 0; // number of devices found.

    struct udev_enumerate *e = udev_enumerate_new(udev);
    if(!e) {
        ERROR("udev_enumerate_new() failed");
        return 0;
    }
    udev_enumerate_add_match_subsystem(e, "usb");
    udev_enumerate_add_match_property(e, "DEVTYPE", "usb_device");
    udev_enumerate_scan_devices(e);
    struct udev_list_entry *devices = udev_enumerate_get_list_entry(e);
    struct udev_list_entry *entry = 0;
    udev_list_entry_foreach(entry, devices) {

	const char *path = udev_list_entry_get_name(entry);
        if(!path) {
            ERROR("udev_list_entry_get_name() failed");
            return 0;
        }
        struct udev_device *udev_dev =
            udev_device_new_from_syspath(udev, path);
        if(!udev_dev) {
            ERROR("udev_device_new_from_syspath(\"%s\") failed\n",
                    path);
            return 0;
        }

        if(strcmp(idVender,
                udev_device_get_sysattr_value(udev_dev, "idVendor")) == 0
                && strcmp(idProduct,
                udev_device_get_sysattr_value(udev_dev, "idProduct")) == 0
                ) {
            INFO("udev found: manufacturer \"%s\", product \"%s\",\n"
                    " configuration \"%s\", devnode \"%s\"",
                    udev_device_get_sysattr_value(udev_dev, "manufacturer"),
                    udev_device_get_sysattr_value(udev_dev, "product"),
                    udev_device_get_sysattr_value(udev_dev, "configuration"),
                    udev_device_get_devnode(udev_dev)
                    );
            ++n;
            ret = realloc(ret, (n+1)*sizeof(*ret));
            ASSERT(ret, "realloc(%p, %zu) failed",
                    ret, (n+1)*sizeof(*ret));
        }

        udev_device_unref(udev_dev);
    }

    udev_enumerate_unref(e);

    return 0;
}


void
qsu_usbdev_delete(const char **devices) {


}
