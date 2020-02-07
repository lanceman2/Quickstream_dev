# 1.  build all [a-z]*.c as a filter module
# 2.  build all [A-Z]*.c as a uninstalled test

INSTALL_DIR = $(PREFIX)/lib/quickstream/plugins/filters/tests

plugins := $(patsubst %.c, %, $(wildcard [a-z]*.c))

define makeSOURCES
$(1)$(2)_SOURCES := $(1).c
endef


BUILD_NO_INSTALL := $(patsubst %.c, %, $(wildcard [A-Z]*.c))


# We make a DSO (dynamic shared object) plugin for each C file
# that is in the glob pattern [a-z]*.c
$(foreach targ,$(plugins),$(eval $(call makeSOURCES,$(targ),.so)))
$(foreach targ,$(BUILD_NO_INSTALL),$(eval $(call makeSOURCES,$(targ),)))



include ../../../../../quickbuild.make
