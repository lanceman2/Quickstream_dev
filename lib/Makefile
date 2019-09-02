
INSTALL_DIR = $(PREFIX)/lib

libquickstream.so_SOURCES :=\
 app.c\
 filter.c\
 stream.c\
 thread.c\
 debug.c\
 opts.c\
 buffer.c
libquickstream.so_LDFLAGS := -lpthread -ldl

# build and install pkgconfig/quickstream.pc
#
# We needed do this now because we do not require PREFIX to be set until
# "make install" is run, for this quickbuild building thingy.  We wanted
# to make building independent of PREFIX.  Not so with the GNU autotools
# building method in "makefile.am".
POST_INSTALL_COMMAND = ./qb_install_quickstream.pc.bash $(PREFIX)



include ../quickbuild.make
