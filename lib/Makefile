
INSTALL_DIR = $(PREFIX)/lib

libquickstream.so_SOURCES :=\
 app.c\
 filter.c\
 filterAPI.c\
 prePostInputCallbacks.c\
 stream.c\
 streamReady.c\
 debug.c\
 opts.c\
 buffer.c\
 flow.c\
 makeRingBuffer.c\
 streamLaunch.c\
 parameter.c\
 controller.c\
 Dictionary.c

libquickstream.so_LDFLAGS := -lpthread -ldl -lrt
# TODO: need to add something like: -export-symbols-regex '^qs'


ifeq ($(shell if pkg-config libudev --exists; then echo yes; fi),yes)
# This library may not even get added to the GNU autotools build
# It's not working yet (as of 2020-06-11).
# It may get removed from quickstream.  It's just too much dam work.
#
# Just wanted to remove the librtlsdr (and libusb) dependency which does
# not code with a UNIX like paradigm, it's like f-ing Windows code on
# GNU/Linux (UNIX).  UNIX has a multiplexing paradigm which does not exist
# on Windows and libusb ignores the UNIX multiplexing paradigm.  Pisses me
# off.
libqsu.so_SOURCES :=\
 qsu_usbdev.c\
 debug.c\
 qsu_usbdev_print.c\
 qsu_rtlsdr.c\
 qsu_rtlsdr_GetDescriptor.c
libqsu.so_LDFLAGS := $(shell pkg-config --libs libudev)
libqsu.so_CFLAGS := $(shell pkg-config --cflags libudev)
endif


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
