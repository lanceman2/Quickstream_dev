# This file, config.make.example.rl, defines possible make configuration
# variables.   This file in an input file to make config.make.example.
#
# We try to keep similar make variables in the GNU autotools build method.

PREFIX = /usr/local/encap/quickstream-@PACKAGE_VERSION@

# Set CPPFLAGS for debug build options:
#
#
# SPEW_LEVEL_* controls macros in lib/debug.h
#
# The following may be defined; defining them turns on the following CPP
# macro functions that are in debug.h which is source for
# libquickstream.so
#
#
# DEBUG             -->  DASSERT()
#
# SPEW_LEVEL_DEBUG  -->  DSPEW() INFO() NOTICE() WARN() ERROR()
# SPEW_LEVEL_INFO   -->  INFO() NOTICE() WARN() ERROR()
# SPEW_LEVEL_NOTICE -->  NOTICE() WARN() ERROR()
# SPEW_LEVEL_WARN   -->  WARN() ERROR()
# SPEW_LEVEL_ERROR  -->  ERROR()
#
# always on is      -->  ASSERT()


# Example:
#CPPFLAGS := -DSPEW_LEVEL_NOTICE
CPPFLAGS := -DDEBUG -DSPEW_LEVEL_DEBUG


# C compiler option flags
CFLAGS := -g -Wall -Werror


########################################################################
# WITH_LIBRTLSDR can be 1) unset, 2) auto, or 3) yes
#
# 1) WITH_LIBRTLSDR unset this will not try to build code that needs
#    librtlsdr
#
# 2) WITH_LIBRTLSDR := auto # will cause make to check for librtlsdr by
#    running:  'pkg-config --exists librtlsdr' and if it succeeds will try
#    to build and install quickstream things that depend on librtlsdr.
#
# 3) WITH_LIBRTLSDR := yes #  will cause make to check for librtlsdr by
#    running:  'pkg-config --exists librtlsdr' and if it fails will cause
#    make to fail; but on success will try to build and install
#    quickstream things that depend on librtlsdr, and if that fails will
#    cause make to fail.
#
WITH_LIBRTLSDR := auto


