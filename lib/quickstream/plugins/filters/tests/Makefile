
INSTALL_DIR = $(PREFIX)/lib/quickstream/plugins/filters/tests

plugins := $(patsubst %.c, %, $(wildcard *.c))

define makePlugins
$(1).so_SOURCES := $(1).c
endef

# We make a DSO (dynamic shared object) plugin for each C file. 
$(foreach targ,$(plugins),$(eval $(call makePlugins,$(targ))))


include ../../../../../quickbuild.make
