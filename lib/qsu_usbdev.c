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
qsu_usbdev_find_new(const char *idVender, const char *idProduct,
        const char *speed) {

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

        const char *idV =
            udev_device_get_sysattr_value(udev_dev, "idVendor");
        const char *idP =
            udev_device_get_sysattr_value(udev_dev, "idProduct");
        const char *sp =
            udev_device_get_sysattr_value(udev_dev, "speed");

        // See if we found what we were looking for:
        if(
                (!idVender || strcmp(idVender, idV) == 0) &&
                (!idProduct || strcmp(idProduct, idP) == 0) &&
                (!speed || strcmp(speed, sp) == 0)
          ) {
            const char *devnode = udev_device_get_devnode(udev_dev);
            INFO("\n udev found match: manufacturer \"%s\", product \"%s\",\n"
                    " configuration \"%s\", devnode \"%s\",\n"
                    " udev_path \"%s\",\n"
                    " speed \"%s\"",
                    udev_device_get_sysattr_value(udev_dev,
                        "manufacturer"),
                    udev_device_get_sysattr_value(udev_dev, "product"),
                    udev_device_get_sysattr_value(udev_dev,
                        "configuration"), devnode, path, sp
                    );
            ASSERT(devnode,
                    "dev_device_get_devnode() failed "
                    "after finding entry");
            ++n;
            ret = realloc(ret, (n+1)*sizeof(*ret));
            ASSERT(ret, "realloc(%p, %zu) failed",
                    ret, (n+1)*sizeof(*ret));
            ret[n-1] = strdup(devnode);
            ASSERT(ret[n-1], "stddup() failed");
            ret[n] = 0; // null terminate.
        }

        udev_device_unref(udev_dev);
    }

    udev_enumerate_unref(e);

    return ret;
}


void qsu_usbdev_find_delete(const char **devices) {
    if(!devices) return;
    // We made the user interface const but we need to cleanup.
    char **d = (char **) devices;
    while(*devices)
        free((char *) *devices++);
    free(d);
}
