#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>

#include "debug.h"
#include "../include/quickstream/qsu.h"

// Reference:
// https://www.beyondlogic.org/usbnutshell/usb5.shtml#InterfaceDescriptors


// Returns bNumConfigurations
//
static uint8_t
printDeviceDescriptor(FILE *f, uint8_t *buf, size_t len) {

    uint8_t ret = 0;

    fprintf(f, "  Device [shape=record  label=\"{"
            "Device"
            );

    if(*buf != 18) {
        fprintf(f, "|BAD bLength=%hhu not 18", *buf);
        fprintf(f, "}\"];\n");
        return len; // eat the whole length
    }
    fprintf(f, "|bLength=%hhu Bytes", *buf);
    ++buf;

    if(*buf != 0x01) {
        fprintf(f, "|BAD bDescriptorType=0x%02hhx not 0x01", *buf);
        fprintf(f, "}\"];\n");
        return len; // eat the whole length
    }
    fprintf(f, "|bDescriptorType=0x%02hhx", *buf);
    ++buf;
    fprintf(f, "|bcdUSB=%hu", *(uint8_t *)buf);
    buf += 2;
    fprintf(f, "|bDeviceClass=%hhu", *(uint8_t *)buf);
    ++buf;
    fprintf(f, "|bDeviceSubClass=%hhu", *(uint8_t *)buf);
    ++buf;
    fprintf(f, "|bDeviceProtocol=%hhu", *(uint8_t *)buf);
    ++buf;
    fprintf(f, "|bMaxPacketSize=%hhu", *(uint8_t *)buf);
    ++buf;
    fprintf(f, "|idVender=0x%04hx", *(uint16_t *)buf);
    buf += 2;
    fprintf(f, "|idProduct=0x%04hx", *(uint16_t *)buf);
    buf += 2;
    fprintf(f, "|bcdDevice=0x%04hx", *(uint16_t *)buf);
    buf += 2;
    fprintf(f, "|iManufacturer=%hhu", *(uint8_t *)buf);
    ++buf;
    fprintf(f, "|iProduct=%hhu", *(uint8_t *)buf);
    ++buf;
    fprintf(f, "|iSerialNumber=%hhu", *(uint8_t *)buf);
    ++buf;
    fprintf(f, "|bNumConfigurations=%hhu", *(uint8_t *)buf);

    ret = *(uint8_t *)buf; //NumConfigurations

    fprintf(f, "}\"];\n");

    return ret;
}

void PrintBits(FILE *f, uint8_t byte) {

    for(int i=0; i<8; ++i)
        fprintf(f, "%hhu", (byte & ( 01 << i)) >> i);
}

// Returns length of buf eaten.
//
static size_t printEndpointDescriptor(FILE *f, uint8_t *buf,
        size_t len, uint8_t cCount, uint8_t iCount, uint8_t eCount) {

    fprintf(f, "  Endpoint%hhu_%hhu_%hhu"
            " [shape=record  label=\"{"
            "Endpoint %hhd %hhu %hhu"
            , cCount, iCount, eCount,
            cCount, iCount, eCount);
    if(*buf != 7) {
        fprintf(f, "|BAD bLength=%hhu not 7", *buf);
        fprintf(f, "}\"];\n");
        return len; // eat the whole length
    }
    fprintf(f, "|bLength=%hhu Bytes", *buf);
    ++buf;

    if(*buf != 0x05) {
        fprintf(f, "|BAD bDescriptorType=0x%02hhx not 0x05", *buf);
        fprintf(f, "}\"];\n");
        return len; // eat the whole length
    }
    fprintf(f, "|bDescriptorType=0x%02hhx", *buf);
    ++buf;
    fprintf(f, "|bEndpointAddress=0x%02hhx", *buf);
    ++buf;
    fprintf(f, "|bmAttributes="); PrintBits(f, *buf);
    ++buf;
    fprintf(f, "|wMaxPacketSize=%hu Bytes", *((uint16_t *)buf));
    buf += 2;
    fprintf(f, "|bInterval=%hhu", *buf);
    ++buf;

    fprintf(f, "}\"];\n");

    fprintf(f, "  Interface%hhu_%hhu -> Endpoint%hhu_%hhu_%hhu;\n",
            cCount, iCount, cCount, iCount, eCount);

    return 7;
}



// Returns length of buf eaten.
//
static size_t printInterfaceDescriptor(FILE *f, uint8_t *buf,
        size_t len, uint8_t cCount, uint8_t iCount) {

    size_t tlen = 0; // total length of buf eaten.
    uint8_t numEndpoints;
    uint8_t eCount = 0;

    fprintf(f, "  Interface%hhu_%hhu"
            " [shape=record  label=\"{"
            "Interface %hhd %hhu"
            , cCount, iCount, cCount, iCount);
    if(*buf != 9) {
        fprintf(f, "|BAD bLength=%hhu not 9", *buf);
        fprintf(f, "}\"];\n");
        return len; // eat the whole length
    }
    fprintf(f, "|bLength=%hhu Bytes", *buf);
    ++buf;

    if(*buf != 0x04) {
        fprintf(f, "|BAD bDescriptorType=0x%02hhx not 0x04", *buf);
        fprintf(f, "}\"];\n");
        return len; // eat the whole length
    }
    fprintf(f, "|bDescriptorType=%02hhx", *buf);
    ++buf;
    fprintf(f, "|bInterfaceNumber=%hhu", *buf);
    ++buf;
    fprintf(f, "|bAlternateSetting=%hhu", *buf);
    ++buf;
    fprintf(f, "|bNumEndpoints=%hhu", numEndpoints = *buf);
    ++buf;
    fprintf(f, "|bInterfaceClass=0x%02hhx", *buf);
    ++buf;
    fprintf(f, "|bInterfaceSubClass=0x%02hhx", *buf);
    ++buf;
    fprintf(f, "|bInterfaceProtocol=%02hhx", *buf);
    ++buf;
    fprintf(f, "|iInterface=%hhi", *buf);
    ++buf;

    fprintf(f, "}\"];\n");

    fprintf(f, "  Configuration%hhd -> Interface%hhu_%hhu;\n",
            cCount, cCount, iCount);

    tlen += 9;
    len -= 9;

    while(numEndpoints && len > 7) {
        size_t r = printEndpointDescriptor(f, buf, len,
                cCount, iCount, eCount++);
        len -= r;
        buf += r;
        tlen += r;
        --numEndpoints;
    }

    return tlen;
}


// Returns length of buf eaten.
//
static size_t
printConfigurationDescriptor(FILE *f, uint8_t *buf,
        size_t len, uint8_t count) {

    uint8_t numInterfaces = 0;
    uint8_t iCount = 0;
    size_t tlen = 0;

    DASSERT(len >= 9);

    fprintf(f, "  Configuration%hhu"
            " [shape=record  label=\"{"
            "Configuration %hhu"
            , count, count);

    if(*buf != 9) {
        fprintf(f, "|BAD bLength=%hhu not 9", *buf);
        fprintf(f, "}\"];\n");
        return len; // eat the whole length
    }
    fprintf(f, "|bLength=%hhu", *buf);
    ++buf;

    if(*buf != 0x02) {
        fprintf(f, "|BAD bDescriptorType=0x%02hhx not 0x02", *buf);
        fprintf(f, "}\"];\n");
        return len; // eat the whole length
    }

    fprintf(f, "|bDescriptorType=0x%hhx", *buf);
    ++buf;
    fprintf(f, "|wTotalLength=%hu bytes", *buf);
    uint16_t wTotalLength = *((uint16_t *) buf);
    buf += 2;
    fprintf(f, "|bNumInterfaces=%hhu", *buf);
    numInterfaces = *buf;
    ++buf;
    fprintf(f, "|bConfigurationValue=%hhu", *buf);
    ++buf;
    fprintf(f, "|iConfiguration=%hhu", *buf);
    ++buf;
    fprintf(f, "|bmAttributes=0%hho", *buf);
    ++buf;
    fprintf(f, "|bMaxPower=%hhux2mA", *buf);
    ++buf;

    tlen += 9;
    len -= 9;

    fprintf(f, "}\"];\n");
    fprintf(f, "  Device -> Configuration%hhd;\n", count);
 
    if(len < numInterfaces * 9) {
        fprintf(f, " ERROR [ label=\"ERROR: Descriptors length is short %zu for "
                "%hhu interfaces (<%d)\"];\n",
                len, numInterfaces, numInterfaces * 9);
        return len; // eat the whole length
    }

    while(numInterfaces && len >= 9) {
        size_t r = printInterfaceDescriptor(f, buf, len, count, iCount++);
        tlen += r;
        len -= r;
        buf += r;
        --numInterfaces;
    }

    if(((size_t) wTotalLength) != tlen)
        fprintf(f, "  Configuation_%hhu [shape=record label=\"{"
                "Configuration %hhu has BAD wTotalLength"
                "=%hu measured %zu}\"];\n",
                count, count, wTotalLength, tlen);
    return tlen;
}



void qsu_usbdev_descriptor_print_dot(FILE *f, const char *path) {

    int dirfd = open(path, 0, O_DIRECTORY);
    if(dirfd < 0) {
        ERROR("open(\"%s\",,) failed", path);
        return;
    }

    int fd = openat(dirfd, "descriptors", O_RDONLY);
    close(dirfd);
    if(fd < 0) {
        ERROR("openat(%d,\"descriptors\",) failed",
                dirfd);
        return; // error
    }

    size_t buflen = 512;
    uint8_t *buf = malloc(buflen);
    ASSERT(buf, "malloc(%zu) failed", buflen);
    uint8_t *ptr = buf;
    size_t len = 0;
    while(true) {
        // read the descriptor into one buffer.
        ssize_t r = read(fd, ptr, buflen-len);
        if(r <= 0) {
            close(fd);
            if(len <= 0 || r < 0)
                return; // error
            else
                break;
        }
        len += r;
        ptr += r;
        if(len == buflen) {
            if(buflen > 1024*1024) {
                ERROR("%s/descriptor appears to be stupid large "
                    "> 1024*1024 bytes", path);
                return;
            }
            buf = realloc(buf, buflen += 512);
            ASSERT(buf, "realloc(%p,%zu) failed",
                buf, buflen);
            ptr = buf + len;
        }
    }

    size_t totalLen = len;
    uint8_t numconfig = 0;

    ptr = buf;

    fprintf(f, "digraph {\n");

    if(len >= 18) {
        numconfig = printDeviceDescriptor(f, ptr, len);
        ptr += 18;
        len -= 18;
    } else {
        fprintf(f, "label=\"USB Descriptors, length %zu"
            " bytes is too small < 18\";\n", len);
        fprintf(f, "}\n");
        free(buf);
        return;
    }

    uint8_t configCount = 0;

    while(numconfig && len >= 10) {

        size_t clen = printConfigurationDescriptor(f,
                ptr, len, configCount++);
        len -= clen;
        ptr += clen;
        --numconfig;
    }

    if(len) {
        // We have extra length of data that does not conform
        // to the USB Descriptors standard.
        fprintf(f, "  Unknown_data [shape=record  label=\"{"
                "Unknown data length %zu Bytes}\"];\n"
                "}\n", len);
        free(buf);
        return;
    }

    fprintf(f, "label=\"USB Descriptors, length %zu"
            " bytes\";\n", totalLen);

    fprintf(f, "}\n");
    free(buf);
}
