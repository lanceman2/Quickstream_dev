#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "debug.h"
#include "../include/quickstream/qsu.h"



static inline bool ckprintU8(FILE *f,
        const char *name, uint8_t rd, uint8_t ck) {
    
    if(rd != ck) {
        fprintf(f, "invalid %s", name);
        return true;
    }
    fprintf(f, "%s=%" PRIu8, name, rd);
    return false;
}

// Returns bNumConfigurations
static uint8_t
printDeviceDescriptor(FILE *f, uint8_t *buf, size_t len) {

    uint8_t ret = 0;

    fprintf(f, "  Device [shape=record  label=\"{"
            "Device|"
            );
    
    if(len < 18) {
        fprintf(f, "invalid Descriptor");
        goto end;
    }

    if(ckprintU8(f, "bLength", *buf++, 18)) goto end;
    fprintf(f, " Bytes|");
    if(ckprintU8(f, "bDescriptorType", *buf++, 1)) goto end;
    fprintf(f, "|");
    fprintf(f, "bcdUSB=%hu", *(uint8_t *)buf);
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

end:

    fprintf(f, "}\"];\n");

    return ret;
}

void PrintBits(FILE *f, uint8_t byte) {

    for(int i=0; i<8; ++i)
        fprintf(f, "%hhu", (byte & ( 01 << i)) >> i);
}

// Returns total length of buf eaten.
//
static size_t printEndpointDescriptor(FILE *f, uint8_t *buf,
        size_t len, uint8_t cCount, uint8_t iCount, uint8_t eCount) {

    fprintf(f, "  Endpoint%hhu_%hhu_%hhu"
            " [shape=record  label=\"{"
            "Endpoint %hhd %hhu %hhu"
            , cCount, iCount, eCount,
            cCount, iCount, eCount);

    fprintf(f, "|bLength=%hhu Bytes", *buf);
    ++buf;
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



// Returns total length of buf eaten.
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

    fprintf(f, "|bLength=%hhu Bytes", *buf);
    ++buf;
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


// Returns total length of buf eaten.
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
            "Configuration %hhu|"
            , count, count);

    if((*buf) != 9) {
        printf("BAD bLength=%hhu != 9", *buf);
        fprintf(f, "}\"];\n");
        return len; // eat the whole length
    }
    fprintf(f, "bLength=%hhu|", *buf);
    ++buf;
    fprintf(f, "bDescriptorType=%hhx|", *buf);
    ++buf;
    fprintf(f, "wTotalLength=%hu bytes|", *buf);
    buf += 2;
    fprintf(f, "bNumInterfaces=%hhu|", *buf);
    numInterfaces = *buf;
    ++buf;
    fprintf(f, "bConfigurationValue=%hhu|", *buf);
    ++buf;
    fprintf(f, "iConfiguration=%hhu|", *buf);
    ++buf;
    fprintf(f, "bmAttributes=0%hho|", *buf);
    ++buf;
    fprintf(f, "bMaxPower=%hhux2mA", *buf);
    ++buf;

    tlen += 9;
    len -= 9;

    fprintf(f, "}\"];\n");
    fprintf(f, "  Device -> Configuration%hhd;\n", count);
 

    if(len < numInterfaces * 9) {
        printf("Descriptors length is short for %hhu interfaces",
                numInterfaces);
        return len; // eat the whole length
    }

    if(len < 9) return len;

    while(numInterfaces && len >= 9) {
        size_t r = printInterfaceDescriptor(f, buf, len, count, iCount++);
        tlen += r;
        len -= r;
        buf += r;
        --numInterfaces;
    }

    return tlen;
}



void qsu_usbdev_descriptor_print_dot(FILE *f, const char *path) {

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
    
    size_t totalLen = len;

    ptr = buf;

    fprintf(f, "digraph {\n");

    uint8_t numconfig = printDeviceDescriptor(f, ptr, len);

    ptr += 18;
    len -= 18;
    uint8_t configCount = 0;

    while(numconfig && len) {

        if(len < 10) break;

        size_t clen = printConfigurationDescriptor(f,
                ptr, len, configCount++);
        len -= clen;
fprintf(stderr, "clen=%zu     len=%zu\n",clen, len);
        ptr += clen;
        --numconfig;
    }

    fprintf(f, "label=\"USB Descriptors, length %zu"
            " bytes %zu unused\";\n", totalLen, len);



    fprintf(f, "}\n");
}
