#include <stdio.h>


// USB isochronous transfer urb  MAX_ISO_PACKETS_PER_URB
// https://www.kernel.org/doc/html/latest/driver-api/usb/URB.html
// https://stackoverflow.com/questions/19593420/isochronous-usb-transfers-confusion#19624959
// https://www.linuxjournal.com/article/8093
// https://www.beyondlogic.org/usbnutshell/usb4.shtml#Isochronous
// https://www.beyondlogic.org/usbnutshell/usb1.shtml


// https://www.beyondlogic.org/usbnutshell/usb1.shtml
//

struct QsuUsbdev {

    const char *devnode;
    const char *path;
};

struct QsuRtlsdr;


extern
const struct QsuUsbdev **
qsu_usbdev_find_new(const char *venderId, const char *productId,
        const char *speed);


extern
void
qsu_usbdev_find_delete(const struct QsuUsbdev **devices);


extern
void
qsu_usbdev_descriptor_print_dot(FILE *file, const char *path);


extern
struct QsuRtlsdr *qsu_rtlsdr_new(void);

extern
void qsu_rtlsdr_delete(struct QsuRtlsdr *rtlsdr);
