
INSTALL_DIR = $(PREFIX)/lib/quickstream/plugins/run

# Looks like python3.8-config is broken it seems to assume that the prefix
# is always /usr/local
# Looks like pkg-config may work.
PY_LIBDIR := $(shell pkg-config python-3.7 --variable=libdir)
PY_CFLAGS := $(shell pkg-config python-3.7 --cflags)
PY_LDFLAGS := $(shell pkg-config python-3.7 --libs) -Wl,-rpath -Wl,$(PY_LIBDIR)


pythonControllerLoader.so_CPPFLAGS := $(PY_CFLAGS)
pythonControllerLoader.so_LDFLAGS := $(PY_LDFLAGS)


pythonController.so_CPPFLAGS := $(PY_CFLAGS)
pythonController.so_LDFLAGS := $(PY_LDFLAGS)



ifneq ($(pythonControllerLoader.so_LDFLAGS),)
pythonControllerLoader.so_SOURCES := pythonControllerLoader.c
pythonController.so_SOURCES := pythonController.c
endif


include ../../../../quickbuild.make
