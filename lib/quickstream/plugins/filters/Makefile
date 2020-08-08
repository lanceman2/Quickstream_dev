SUBDIRS := tests librtlsdr uhd

INSTALL_DIR = $(PREFIX)/lib/quickstream/plugins/filters

stdin.so_SOURCES := stdin.c
stdout.so_SOURCES := stdout.c

# Not working yet.
#rtl_sdr_rx.so_SOURCES := rtl_sdr_rx.c
# use filter librtlsdr/iq instead

nullSink.so_SOURCES := nullSink.c
uint8ToFloat.so_SOURCES := uint8ToFloat.c


ifeq ($(shell if pkg-config fftw3 --exists; then echo yes; fi),yes)
fftwComplexFloat1D.so_SOURCES := fftwComplexFloat1D.c
fftwComplexFloat1D.so_CFLAGS := $(shell pkg-config --cflags fftw3f)
# Odd that the pkg-config --cflags fftw3 does not include (-lm) math
fftwComplexFloat1D.so_LDFLAGS := $(shell pkg-config --libs fftw3f) -lm
endif


include ../../../../quickbuild.make
