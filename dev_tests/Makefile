# This directory is strictly for testing and development.
#
# Maybe this directory should go away and be replaced by tests/ when this
# software package matures.

# Nothing in this directory should be installed.

# If this directory is not recursed into from the directory above this we
# recurse up a directory if make was run in this directory.
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

test_qsError_SOURCES := test_qsError.c
test_qsError_LDFLAGS := -L../lib -lquickstream -Wl,-rpath=\$$ORIGIN/../lib

RTL-SDR_dot_SOURCES := RTL-SDR_dot.c
RTL-SDR_dot_LDFLAGS := -L../lib -lqsu -Wl,-rpath=\$$ORIGIN/../lib

RTL-SDR_read_SOURCES := RTL-SDR_read.c
RTL-SDR_read_LDFLAGS := -L../lib -lqsu -Wl,-rpath=\$$ORIGIN/../lib

MurmurHash_test_SOURCES := MurmurHash_test.c ../lib/MurmurHash1.c

Dictionary_test_SOURCES := Dictionary_test.c ../lib/Dictionary.c ../lib/debug.c

clock_gettime_SOURCES := clock_gettime.c


regex_SOURCES := regex.c


PY_CPPFLAGS := $(shell python3-config --includes)
PY_LDFLAGS := $(shell python3-config --embed --libs)


pythonRun_SOURCES := pythonRun.c
pythonRun_CPPFLAGS := $(PY_CPPFLAGS)
pythonRun_LDFLAGS := $(PY_LDFLAGS)



# Add extra clean files for this directory.
CLEANFILES := $(wildcard *.tmp)

CLEANERDIRS :=\
 $(wildcard scc-snapshots-*_src)\
 $(wildcard valgrind-*try-*)

CLEANERFILES :=\
 $(wildcard scc-snapshots-*.tgz)\
 $(wildcard valgrind-*.tar.bz2)


include ../quickbuild.make
