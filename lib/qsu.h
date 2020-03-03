
struct Endpoint {

    // Kind of like
    //struct usb_endpoint_descriptor endpoint;
    // but we do not have the so called Audio extension
    // See /usr/include/linux/usb/ch9.h

    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
};


struct Interface {

    struct usb_interface_descriptor interface;

    // Array of size interface.bNumEndpoints
    struct Endpoint *endpoints;
};


struct Config {

    struct usb_config_descriptor config;

    struct Interface *interfaces;
};


struct Descriptor {

    // We inherit usb_device_descriptor
    struct usb_device_descriptor device;

    // This is an allocated array of size descriptor.bNumConfigurations.
    struct Config *configs;
};


// This is the opaque pointer seen by the qsu_rtlsdr_new()
// user interface.
struct QsuRtlsdr {
    int fd;

    // The device is the root of the descriptor tree data structure.
    // Reference:
    // https://www.beyondlogic.org/usbnutshell/usb1.shtml
    //
    struct Descriptor descriptor;
};
