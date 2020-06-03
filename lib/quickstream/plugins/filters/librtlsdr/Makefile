root := ../../../../..

# It's not easy to make build options with quickbuild so we just "automate
# out" the "WITH_LIBRTLSDR make option" by this if switch which just cuts
# out just about the full make file if package librtlsdr is not found.
#
ifeq ($(shell if pkg-config librtlsdr --exists; then echo yes; fi),yes)


# 1.  build all [a-z]*.c as a filter module
# 2.  build all [a-z]*.cpp as a filter module
# 3.  build all [A-Z]*.c as a uninstalled test


INSTALL_DIR = $(PREFIX)/lib/quickstream/plugins/filters/librtlsdr


c_plugins := $(patsubst %.c, %, $(wildcard [a-z]*.c))

cpp_plugins := $(patsubst %.cpp, %, $(wildcard [a-z]*.cpp))


define makeSOURCES
$(1)$(2)_SOURCES := $(1)$(3)
endef


define ADDlib
$(1)_LDFLAGS := -L$(root)/lib -lquickstream -Wl,-rpath=\$$$$ORIGIN/$(root)/lib
endef

LIBRTLSDR_LIBS := $(shell pkg-config librtlsdr --libs)
LIBRTLSDR_CFLAGS := $(shell pkg-config librtlsdr --cflags)

iq.so_LDFLAGS := $(LIBRTLSDR_LIBS)
iq.so_CFLAGS := $(LIBRTLSDR_CFLAGS)

BUILD_NO_INSTALL := $(patsubst %.c, %, $(wildcard [A-Z]*.c))


$(foreach targ,$(c_plugins),$(eval $(call makeSOURCES,$(targ),.so,.c)))
$(foreach targ,$(cpp_plugins),$(eval $(call makeSOURCES,$(targ),.so,.cpp)))
$(foreach targ,$(BUILD_NO_INSTALL),$(eval $(call makeSOURCES,$(targ),,.c)))
$(foreach targ,$(BUILD_NO_INSTALL),$(eval $(call ADDlib,$(targ),)))


endif # ifeq ($(shell if pkg-config librtlsdr ... bla bla bla.

include $(root)/quickbuild.make

