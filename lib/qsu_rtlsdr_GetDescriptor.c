#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <linux/usb/ch9.h>


#include "../include/quickstream/qsu.h"
#include "qsu.h"
#include "debug.h"


void
_qsu_rtlsdr_FreeConfig(struct Config *config) {

    if(config->interfaces == 0) return;

    for(uint8_t i=config->config.bNumInterfaces - 1;
            i != (uint8_t) -1; --i)
        if(config->interfaces[i].endpoints)
            // Free this array of endpoints
            free(config->interfaces[i].endpoints);
    // Free this array of interfaces
    free(config->interfaces);
    config->interfaces = 0;
}


// len is at least as long as a 
// config pointer is not cleaned here.
//
// returns the remaining length or 0 on error.
//
// Otherwise returns many pointers to malloc() and calloc()
// allocated memory.
//
static size_t
GetConfig(struct Config *config, uint8_t *buf, size_t len) {

    size_t len_in = len;
    memcpy(config, buf, USB_DT_CONFIG_SIZE);
    buf += USB_DT_CONFIG_SIZE;
    len -= USB_DT_CONFIG_SIZE;

    if(config->config.bLength != USB_DT_CONFIG_SIZE ||
           config->config.bDescriptorType != USB_DT_CONFIG) {
        ERROR("Invalid USB Configuration Descriptor");
        return 0; // fail
    }

    uint8_t numInterfaces = config->config.bNumInterfaces;

    if(numInterfaces == 0 || numInterfaces > 9) {
        ERROR("At least one USB configuration has %hhu "
                "interfaces in the device",
                numInterfaces);
        return 0; // fail
    }

    config->interfaces = calloc(numInterfaces,
            sizeof(*config->interfaces));
    ASSERT(config->interfaces, "calloc(%hhu,%zu) failed",
            numInterfaces, sizeof(*config->interfaces));

    for(uint8_t i=0; i<numInterfaces; ++i) {
        struct Interface *interface = config->interfaces + i;
        memcpy(interface, buf, USB_DT_INTERFACE_SIZE);
        buf += USB_DT_INTERFACE_SIZE;
        len -= USB_DT_INTERFACE_SIZE;
        uint8_t numEndpoints = interface->interface.bNumEndpoints;

        if(numEndpoints > 10 || interface->interface.bLength !=
                USB_DT_INTERFACE_SIZE ||
                interface->interface.bDescriptorType !=
                USB_DT_INTERFACE || numEndpoints * 7 > len) {
            ERROR("Found bad USB Interface Descriptor"
                    " interface.bLength=%hhu"
                    " bNumEndpoints=%hhu, "
                    " interface.bDescriptorType=0x%02hhx"
                    " len=%zu", interface->interface.bLength,
                    numEndpoints, 
                    interface->interface.bDescriptorType,
                    len);
            // So we do not clean an endpoint that is not there yet.
            memset(interface, 0, USB_DT_INTERFACE_SIZE);
            return 0; // fail
        }

        if(numEndpoints) {
            struct Endpoint *endpoint =
                config->interfaces[i].endpoints =
                    calloc(numEndpoints, sizeof(*endpoint));
            ASSERT(endpoint, "calloc(%hhu,%zu) failed", numEndpoints,
                    sizeof(*endpoint));

            for(uint8_t j=0; j<numEndpoints; ++j) {

                // We cannot use memcpy() this time.
                endpoint->bLength = *buf++;
                endpoint->bDescriptorType = *buf++;
                endpoint->bEndpointAddress = *buf++;
                endpoint->bmAttributes = *buf++;
                endpoint->wMaxPacketSize = *((uint16_t *) buf);
                buf += 2;
                endpoint->bInterval = *buf++;
                len -= 7;

                if(endpoint->bLength != 7 || endpoint->bDescriptorType
                        != USB_DT_ENDPOINT) {
                    ERROR("Found bad USB Endpoint Descriptor %hhu "
                            "endpoint->bLength=%hhu "
                            "endpoint->bDescriptorType=0x%02hhu ",
                            j,
                            endpoint->bLength, endpoint->bDescriptorType);
                    return 0;
                }

                ++endpoint;
            }
        }
    }

    return len_in - len; // success
}


// Returns 0 on error.
static
struct Config *GetConfigs(uint8_t *buf, size_t len,
        uint8_t numConfigs/*not 0*/) {

    DASSERT(USB_DT_CONFIG_SIZE == sizeof(struct usb_config_descriptor));

    if(len < numConfigs * USB_DT_CONFIG_SIZE) {
        ERROR("Invalid Device Descriptor");
        return 0; // fail
    }

    struct Config *configs = calloc(numConfigs, sizeof(*configs));
    ASSERT(configs, "calloc(%hhu,%zu) failed",
            numConfigs, sizeof(*configs));
    uint8_t i=0;
    for(; i<numConfigs && len; ++i) {
        if(len < USB_DT_CONFIG_SIZE)
            break;
        size_t l = GetConfig(configs + i, buf, len);
        buf += l;
        len -= l;
        if(l == 0)
            // error
            break;
    }

    if(i<numConfigs) {
        // cleanup and fail
        while(i != (uint8_t) -1)
            // Free the children.
            _qsu_rtlsdr_FreeConfig(&configs[i--]);
        free(configs);
        return 0;
    }

    return configs;
}


// Returns true on error.
//
// Creates a data structure that contains all the data in the USB devices
// USB Descriptors
//
// https://www.beyondlogic.org/usbnutshell/usb5.shtml#DeviceDescriptors
//
bool _qsu_rtlsdr_GetDescriptor(struct Descriptor *desc,
        const char *path) {

    DASSERT(desc);

    // Read the USBFS Descriptor from the file $path/descriptors
    int dirfd = open(path, 0, O_DIRECTORY);
    if(dirfd < 0) {
        ERROR("open(\"%s\",,) failed", path);
        return true;
    }
    int fd = openat(dirfd, "descriptors", O_RDONLY);
    close(dirfd);
    if(fd < 0) {
        ERROR("openat(%d,\"descriptors\",) failed",
                dirfd);
        return true; // error
    }

    size_t buflen = 512;
    uint8_t *buf = malloc(buflen);
    ASSERT(buf, "malloc(%zu) failed", buflen);
    uint8_t *ptr = buf;
    size_t len = 0;
    while(true) {
        // read the descriptor data into one buffer.
        ssize_t r = read(fd, ptr, buflen-len);
        if(r <= 0) {
            close(fd);
            if(len <= 0 || r < 0) {
                ERROR("Cannot read %s/descriptors",
                        path);
                free(buf);
                return true; // read error
            } else
                break;
        }
        len += r;
        ptr += r;
        if(len == buflen) {
            if(buflen > 1024*1024) {
                ERROR("%s/descriptor appears to be stupid large "
                    "> 1024*1024 bytes", path);
                return true;
            }
            buf = realloc(buf, buflen += 512);
            ASSERT(buf, "realloc(%p,%zu) failed",
                buf, buflen);
            ptr = buf + len;
        }
    }
    close(fd);

    ptr = buf;

    if(len < sizeof(desc->device)) {
        ERROR("%s/descriptor appears too small %zu < %zu",
                path, len, sizeof(desc->device));
        free(buf);
        return true;
    }

    memcpy(&desc->device, buf, sizeof(desc->device));
    // This will be the configs array that we have not allocated yet.
    desc->configs = 0;

    if(desc->device.bNumConfigurations > 3 ||
            desc->device.bNumConfigurations == 0) {
        // Not a device that we expected in this code.
        ERROR("%s/descriptor appears to have %hhu "
                "configurations", path,
                desc->device.bNumConfigurations);
        free(buf);
        return true; // fail
    }

    desc->configs = GetConfigs(buf + sizeof(desc->device),
            len - sizeof(desc->device),
            desc->device.bNumConfigurations);
    free(buf);

    if(!desc->configs) {
        // This could be a valid USB Descriptor but we cannot use it.
        return true; // fail
    }

    return false; // success
}
