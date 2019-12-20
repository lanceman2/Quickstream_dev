#! This is a "GNU make" make file.


ROOT := ../..

BIN := $(patsubst %.c,%,$(wildcard *.c))
LDFLAGS := -L$(ROOT)/lib -lquickstream -Wl,-rpath=\$$ORIGIN/$(ROOT)/lib

build: $(BIN)


$(BIN): %: %.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)


cleaner: clean
clean:
	rm -f *.tmp *.o $(BIN)


# This directory is strictly for testing and development.
#
# Nothing in this directory should be installed.


include $(ROOT)/config.make
