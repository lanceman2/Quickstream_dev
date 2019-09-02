# This directory is strictly for testing and development.
#
# Maybe this directory should go away and be replaced by tests/ when this
# software package matures.

# Nothing in this directory should be installed.

# This directory is not recursed into from the directory above this, so we
# recurse into these directories so that we stay consistent with the
# source in these directories.   We are testing files in ../lib/,
# ../include/, and ../lib/quickstream/plugins/filters by linking and
# loading with generated files there-in.
#
SUBDIRS := ../include ../lib ../lib/quickstream/plugins/filters


run_SOURCES := run.c
run_LDFLAGS := -L../lib -lquickstream -Wl,-rpath=\$$ORIGIN/../lib

run2_SOURCES := run2.c
run2_LDFLAGS := -L../lib -lquickstream -Wl,-rpath=\$$ORIGIN/../lib

test_qsError_SOURCES := test_qsError.c
test_qsError_LDFLAGS := -L../lib -lquickstream -Wl,-rpath=\$$ORIGIN/../lib



include ../quickbuild.make
