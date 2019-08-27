
INSTALL_DIR = $(PREFIX)/include

INSTALLED := qsfilter.h qsapp.h

# Tell quickbuild that RELEASE_VERSION is a string that is sed substituted
# in @RELEASE_VERSION@ in *.in files like qsapp.h.in
#
IN_VARS := RELEASE_VERSION


qsapp.h: qsapp.h.in ../package.make



include ../quickbuild.make
