SUBDIRS := tests librtlsdr

INSTALL_DIR = $(PREFIX)/lib/quickstream/plugins/filters

stdin.so_SOURCES := stdin.c
stdout.so_SOURCES := stdout.c
# Not working yet.
#rtl_sdr_rx.so_SOURCES := rtl_sdr_rx.c
nullSink.so_SOURCES := nullSink.c
uint8ToFloat.so_SOURCES := uint8ToFloat.c


include ../../../../quickbuild.make
