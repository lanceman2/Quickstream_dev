root := ../../../..


INSTALL_DIR = $(PREFIX)/lib/quickstream/plugins/run

#PYTHON3_FROM_SOURCE := yes

# Some (maybe all) code in this directory depends on libpython3 or
# whatever it's called.   If we can't find it we need not build some
# things.


# This is a mess because the if python3 is installed from source the
# "pkg-config python3" outputs are not compatible to how the are from when
# we install from source.

# PY_VER := 3.8 or whatever.
#
PY_VER := $(shell pkg-config python3 --modversion)


# Looks like python3-config is broken.  It seems to assume that the prefix
# is always /usr/local, WTF.
#
# Looks like pkg-config works, but not like it should.  This sucks; it's
# hard to believe that this poor quality of code exists as one of the core
# parts of most GNU/Linux distros.  This kind of bug should have high
# priority (end rant).


#In case we use the C++ code in pyQsController.cpp:
#PYBIND11_CPPFLAGS := $(shell python3 -m pybind11 --includes)


ifdef PYTHON3_FROM_SOURCE

PY_LIBDIR := $(shell pkg-config python-$(PY_VER) --variable=libdir)
PY_CFLAGS := $(shell pkg-config python-$(PY_VER) --cflags)
PY_LDFLAGS := -L$(PY_LIBDIR) -lpython$(PY_VER) -Wl,-rpath -Wl,$(PY_LIBDIR)

else

PY_CFLAGS := $(shell pkg-config python-$(PY_VER) --cflags)
PY_LDFLAGS := -lpython$(PY_VER)

endif


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


# If we build libpython3 dependent thingys at all:
ifneq ($(PY_VER),)
# We will build this stuff that depends on libpython3*
pyQsController.so_SOURCES := pyQsController.c
pythonControllerLoader.so_SOURCES := pythonControllerLoader.c
pythonController.so_SOURCES := pythonController.c
CLEANFILES := pythonController.c
else
# We will not build things that depend on libpython3 in this directory.
$(warning pkg-config did not find libpython-VERSION -\
 pythonControllerLoader.so, pyQsController.so\
 and pythonController.so will not be built)
endif


include ../../../../quickbuild.make
