#! This is a "GNU make" make file.

# This directory is strictly for testing and development.
#
# Nothing in this directory should be installed.

ROOT := ../..

BIN := $(patsubst %.c,%,$(wildcard *.c))
LDFLAGS := -L$(ROOT)/lib -lquickstream -Wl,-rpath=\$$ORIGIN/$(ROOT)/lib

build: $(BIN)


$(BIN): %: %.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)


cleaner: clean
clean:
	rm -f *.tmp *.o $(BIN)


-include $(ROOT)/config.make
