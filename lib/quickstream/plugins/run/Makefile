
INSTALL_DIR = $(PREFIX)/lib/quickstream/plugins/run



pythonControllerLoader.so_CPPFLAGS := $(shell python3-config --includes)
pythonControllerLoader.so_LDFLAGS := $(shell python3-config --embed --libs)


ifneq ($(pythonControllerLoader.so_LDFLAGS),)
pythonControllerLoader.so_SOURCES := pythonControllerLoader.c
endif


include ../../../../quickbuild.make
