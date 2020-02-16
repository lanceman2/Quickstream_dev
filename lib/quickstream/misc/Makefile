
INSTALL_DIR = $(PREFIX)/lib/quickstream/misc

quickstreamHelp_SOURCES := quickstreamHelp.c

BUILD_NO_INSTALL := qsOptions.h

qsOptions.h: quickstreamHelp
	./quickstreamHelp -c > $@

include ../../../quickbuild.make
