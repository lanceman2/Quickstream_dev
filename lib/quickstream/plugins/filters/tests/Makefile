# 1.  build all [a-z]*.c as a filter module
# 2.  build all [A-Z]*.c as a uninstalled test

root := ../../../../..

INSTALL_DIR = $(PREFIX)/lib/quickstream/plugins/filters/tests

plugins := $(patsubst %.c, %, $(wildcard [a-z]*.c))

define makeSOURCES
$(1)$(2)_SOURCES := $(1).c
endef


define ADDlib
$(1)_LDFLAGS := -L$(root)/lib -lquickstream -Wl,-rpath=\$$$$ORIGIN/$(root)/lib
endef

BUILD_NO_INSTALL :=\
 $(patsubst %.c, %, $(wildcard [A-Z]*.c))\
 $(patsubst %.c, %, $(wildcard [A-Z]*.cpp))


# We make a DSO (dynamic shared object) plugin for each C file
# that is in the glob pattern [a-z]*.c
$(foreach targ,$(plugins),$(eval $(call makeSOURCES,$(targ),.so)))
$(foreach targ,$(BUILD_NO_INSTALL),$(eval $(call makeSOURCES,$(targ),)))
$(foreach targ,$(BUILD_NO_INSTALL),$(eval $(call ADDlib,$(targ),)))


include ../../../../../quickbuild.make
