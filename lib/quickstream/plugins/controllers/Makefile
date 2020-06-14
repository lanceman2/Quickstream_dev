SUBDIRS := tests


INSTALL_DIR = $(PREFIX)/lib/quickstream/plugins/controllers


bytesCounter.so_SOURCES := bytesCounter.c

bytesRate.so_SOURCES := bytesRate.c


python.so_CPPFLAGS := $(shell python3-config --includes)
python.so_LDFLAGS := $(shell python3-config --embed --libs)

ifneq ($(python.so_LDFLAGS),)
python.so_SOURCES := python.c
endif



include ../../../../quickbuild.make
