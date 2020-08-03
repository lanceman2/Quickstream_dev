root := ../../../..


INSTALL_DIR = $(PREFIX)/lib/quickstream/plugins/run

# PY_VER := 3.8 or whatever.
#
PY_VER := $(shell pkg-config python3 --modversion)

# Looks like python3-config is broken.  It seems to assume that the
# prefix is always /usr/local
#
# Looks like pkg-config may work, but not like it should.
#
PY_LIBDIR := $(shell pkg-config python-$(PY_VER) --variable=libdir)
PY_CFLAGS := $(shell pkg-config python-$(PY_VER) --cflags)
PY_LDFLAGS := -L$(PY_LIBDIR) -lpython$(PY_VER) -Wl,-rpath -Wl,$(PY_LIBDIR)

PYBIND11_CPPFLAGS := $(shell python3 -m pybind11 --includes)



pythonControllerLoader.so_CPPFLAGS := $(PY_CFLAGS)
pythonControllerLoader.so_LDFLAGS := $(PY_LDFLAGS)


pyQsController.so_CPPFLAGS := $(PY_CFLAGS) $(PYBIND11_CPPFLAGS)
pyQsController.so_LDFLAGS := $(PY_LDFLAGS)\
 -L$(root)/lib -lquickstream -Wl,-rpath=\$$ORIGIN/$(root)/lib


pythonController.so_CPPFLAGS := $(PY_CFLAGS)
pythonController.so_LDFLAGS := $(PY_LDFLAGS)

PYTHON_BUILTIN_MODULE_PATH :=\
 $(shell pkg-config python3 --variable=libdir)/python$(PY_VER)

IN_VARS := PYTHON_BUILTIN_MODULE_PATH


ifneq ($(PYBIND11_CPPFLAGS),)
pyQsController.so_SOURCES := pyQsController.c
pythonControllerLoader.so_SOURCES := pythonControllerLoader.c
pythonController.so_SOURCES := pythonController.c
CLEANFILES := pythonController.c
else
$(warning pkg-config did not find libpython-VERSION -\
 pythonControllerLoader.so, pyQsController.so\
 and pythonController.so will not be built)
endif


include ../../../../quickbuild.make
