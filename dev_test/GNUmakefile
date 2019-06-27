# This directory is strictly for testing and development.
#
# Maybe this directory should go away and be replaced by tests/ when this
# software package matures.

# Nothing in this directory should be installed.

# This directory is not recured into from the directory above this, so we
# recurse into these directories so that we stay consistent with the
# source in these directories.   We are testing files in ../lib/ and
# ../include/ by linking and loading with generated files there-in.
#
SUBDIRS := ../lib ../include


fsrun_SOURCES := fsrun.c
fsrun_LDFLAGS := -L../lib -lfs -Wl,-rpath=\$$ORIGIN/../lib


include ../quickbuild.make
