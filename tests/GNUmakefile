#! This is a "GNU make" make file.

# If this directory is not recursed into from the directory above this we
# recurse up a directory if make was run in this directory.
ifeq ($(strip $(MAKELEVEL)),0)
# We do not try to build in .. if this is using GNU autotools.
ifeq ($(wildcard ../makefile)),)
SUBDIRS := ..
endif
endif

SUBDIRS += filters

BUILD_NO_INSTALL := tests/tests 301_Dictionary_test


# This also tests that ../lib/Dictionary.c can be used
# without linking with libquickstream.so
301_Dictionary_test_SOURCES := 301_Dictionary_test.c ../lib/Dictionary.c ../lib/debug.c


# Just tests that debug.c is independent of other files,
# and so also does not depend on libquickstream.so
021_debug_SOURCES := 021_debug.c ../lib/debug.c


ifeq ($(wildcard ../makefile)),)
# Needed only for testing with autotools building/testing.
tests/tests:
	ln -fs ../lib/quickstream/plugins/filters/tests/.libs tests/tests
endif

CLEANFILES := $(wildcard *.tmp)

CLEANERFILES := tests/tests

include ../quickbuild.make
