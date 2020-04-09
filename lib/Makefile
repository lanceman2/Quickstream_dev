
INSTALL_DIR = $(PREFIX)/lib

libquickstream.so_SOURCES :=\
 app.c\
 filter.c\
 filterAPI.c\
 stream.c\
 debug.c\
 opts.c\
 buffer.c\
 flow.c\
 makeRingBuffer.c\
 streamLaunch.c\
 parameter.c\
 controller.c\
 Dictionary.c


libqsu.so_SOURCES :=\
 qsu_usbdev.c\
 debug.c\
 qsu_usbdev_print.c\
 qsu_rtlsdr.c\
 qsu_rtlsdr_GetDescriptor.c
libqsu.so_LDFLAGS := $(shell pkg-config --libs libudev)
libqsu.so_CFLAGS := $(shell pkg-config --cflags libudev)


libquickstream.so_LDFLAGS := -lpthread -ldl -lrt
# TODO: need to add something like: -export-symbols-regex '^qs'

# build and install pkgconfig/quickstream.pc
#
# We needed do this now because we do not require PREFIX to be set until
# "make install" is run, for this quickbuild building thingy.  We wanted
# to make building independent of PREFIX.  Not so with the GNU autotools
# building method in "makefile.am".
POST_INSTALL_COMMAND = ./qb_install_quickstream.pc.bash $(PREFIX)


include ../quickbuild.make
