
INSTALL_DIR = $(PREFIX)/lib/quickstream/plugins/run

# Looks like python3.8-config is broken.  It seems to assume that the
# prefix is always /usr/local
#
# Looks like pkg-config may work, but not like it should.
PY_LIBDIR := $(shell pkg-config python-3.8 --variable=libdir)
PY_CFLAGS := $(shell pkg-config python-3.8 --cflags)
PY_LDFLAGS := -L$(PY_LIBDIR) -lpython3.8 -Wl,-rpath -Wl,$(PY_LIBDIR)


pythonControllerLoader.so_CPPFLAGS := $(PY_CFLAGS)
pythonControllerLoader.so_LDFLAGS := $(PY_LDFLAGS)


pythonController.so_CPPFLAGS := $(PY_CFLAGS)
pythonController.so_LDFLAGS := $(PY_LDFLAGS)

PYTHON_BUILTIN_MODULE_PATH :=\
 $(shell pkg-config python3 --variable=libdir)/python$(shell pkg-config python3 --modversion)

IN_VARS := PYTHON_BUILTIN_MODULE_PATH


ifneq ($(PY_LIBDIR),)
pythonControllerLoader.so_SOURCES := pythonControllerLoader.c pyQsController.c
pythonController.so_SOURCES := pythonController.c
else
$(warning pkg-config did not find libpython-3.8 -\
 pythonControllerLoader.so and pythonController.so will not be built)
endif


include ../../../../quickbuild.make
