
INSTALL_DIR = $(PREFIX)/lib/quickstream/plugins/run


PY_CFLAGS := $(shell python3-config --includes)
PY_LDFLAGS := $(shell python3-config --embed --libs)


pythonControllerLoader.so_CPPFLAGS := $(PY_CFLAGS)
pythonControllerLoader.so_LDFLAGS := $(PY_LDFLAGS)

pythonController.so_CPPFLAGS := $(PY_CFLAGS)
pythonController.so_LDFLAGS := $(PY_LDFLAGS)



ifneq ($(pythonControllerLoader.so_LDFLAGS),)
pythonControllerLoader.so_SOURCES := pythonControllerLoader.c
pythonController.so_SOURCES := pythonController.c
endif


include ../../../../quickbuild.make
