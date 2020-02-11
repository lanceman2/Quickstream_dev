#! This is a "GNU make" make file.

# If this directory is not recursed into from the directory above this we
# recurse up a directory if make was run in this directory.
ifeq ($(strip $(MAKELEVEL)),0)
SUBDIRS := ..
endif

SUBDIRS += filters


CLEANFILES := $(wildcard *.tmp)

include ../quickbuild.make
