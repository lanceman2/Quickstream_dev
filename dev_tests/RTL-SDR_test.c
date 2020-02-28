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



static bool ckprintU8(const char *name, uint8_t rd, uint8_t ck) {
    
    if(rd != ck) {
        printf("invalid %s", name);
        return true;
    }
    printf("%s %" PRIx8, name, rd);
    return false;
}

// Returns bNumConfigurations
static uint8_t
printDeviceDescriptor(uint8_t *buf, size_t len) {

    uint8_t ret = 0;

    printf("  Device [shape=record  label=\"{"
            "Device|"
            );
    
    if(len < 18) {
        printf("invalid Descriptor");
        goto end;
    }

    if(ckprintU8("bLength", *buf++, 18)) goto end;
    printf("|");
    if(ckprintU8("bDescriptorType", *buf++, 1)) goto end;
    printf("|");
    printf("bcdUSB %hu", *(uint8_t *)buf);
    buf += 2;
    printf("|bDeviceClass %hhu", *(uint8_t *)buf);
    ++buf;
    printf("|bDeviceSubClass %hhu", *(uint8_t *)buf);
    ++buf;
    printf("|bDeviceProtocol %hhu", *(uint8_t *)buf);
    ++buf;
    printf("|bMaxPacketSize %hhu", *(uint8_t *)buf);
    ++buf;
    printf("|idVender 0x%04hx", *(uint16_t *)buf);
    buf += 2;
    printf("|idProduct 0x%04hx", *(uint16_t *)buf);
    buf += 2;
    printf("|bcdDevice 0x%04hx", *(uint16_t *)buf);
    buf += 2;
    printf("|iManufacturer %hhu", *(uint8_t *)buf);
    ++buf;
    printf("|iProduct %hhu", *(uint8_t *)buf);
    ++buf;
    printf("|iSerialNumber %hhu", *(uint8_t *)buf);
    ++buf;
    printf("|bNumConfigurations %hhu", *(uint8_t *)buf);

    ret = *(uint8_t *)buf; //NumConfigurations

end:

    printf("}\"];\n");

    return ret;
}

static size_t
printConfigurationDescriptor(uint8_t *buf,
        size_t len, uint8_t count) {

    printf("  Configuration%hhd"
            " [shape=record  label=\"{"
            "Configuration %hhd|"
            , count, count);

    printf("}\"];\n");

    printf("  Device -> Configuration%hhd\n", count);
    
    return len;
}



static void printUSBDescriptor(const char *path) {


    int dirfd = open(path, 0, O_DIRECTORY);
    if(dirfd < 0) return;

    int fd = openat(dirfd, "descriptors", O_RDONLY);
    close(dirfd);
    if(fd < 0) return; // error

    uint8_t buf[1024];
    uint8_t *ptr = buf;
    size_t len = 0;
    while(true) {
        ssize_t r = read(fd, ptr, 1024-len);
        if(r <= 0) {
            close(fd);
            if(len <= 0 || r < 0)
                return; // error
            else
                break;
        }
        len += r;
        ptr += r;
        if(len == 1024) {
            ERROR("We need a larger buffer here");
            close(fd);
            return; // error
        }
    }

    ptr = buf;

    printf("digraph {\n");

    printf("label=\"USB Descriptors, length %zu bytes\";\n", len);


    uint8_t numconfig = printDeviceDescriptor(ptr, len);

    ptr += 18;
    len -= 18;
    uint8_t configCount = 0;

    while(numconfig && len) {

        size_t clen = printConfigurationDescriptor(
                ptr, len, configCount++);
        len -= clen;
        ptr += clen;
        --numconfig;
    }


    printf("}\n");
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

    printUSBDescriptor(devs[0]->path);

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
