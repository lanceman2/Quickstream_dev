# This directory is strictly for testing and development.
#
# Maybe this directory should go away and be replaced by tests/ when this
# software package matures.

# Nothing in this directory should be installed.

# If this directory is not recursed into from the directory above this
# we recurse up a directory.
ifeq ($(strip $(MAKELEVEL)),0)
SUBDIRS := ..
endif


getOpts_SOURCES := getOpts.c ../bin/getOpt.c
getOpt.o_CPPFLAGS := -I../bin
getOpts_LDFLAGS := -L../lib -lquickstream -Wl,-rpath=\$$ORIGIN/../lib

assert_SOURCES := assert.c
assert_LDFLAGS := -L../lib -lquickstream -Wl,-rpath=\$$ORIGIN/../lib

run_SOURCES := run.c
run_LDFLAGS := -L../lib -lquickstream -Wl,-rpath=\$$ORIGIN/../lib

run2_SOURCES := run2.c
run2_LDFLAGS := -L../lib -lquickstream -Wl,-rpath=\$$ORIGIN/../lib

test_qsError_SOURCES := test_qsError.c
test_qsError_LDFLAGS := -L../lib -lquickstream -Wl,-rpath=\$$ORIGIN/../lib


# Add extra clean files for this directory.
CLEANFILES := $(wildcard *.tmp)


CLEANERFILES := $(wildcard scc-snapshots-*.tgz)
CLEANERDIRS := $(wildcard scc-snapshots-*_src)


include ../quickbuild.make
