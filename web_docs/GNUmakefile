# This is a GNU make make file.

# We just let this complain if the TARGET_DIR is not present.
#
# This make file just writes to file in TARGET_DIR.
#

TARGET_DIR := ../../faststream.doc

BUILD :=\
 $(patsubst %.dot, $(TARGET_DIR)/%.png, $(wildcard *.dot))\
 $(patsubst %.dot, $(TARGET_DIR)/%.svg, $(wildcard *.dot))


build: $(BUILD)

# We need to look at the images every time we create them so why not just
# script that here and make with an interactive make process using
# "display" in the next rule below.

$(TARGET_DIR)/%.png: %.dot
	dot $< -T png > $@
	display $@ &

$(TARGET_DIR)/%.svg: %.dot
	dot $< -T svg > $@



# It's not a good idea to run clean is TARGET_DIR is in a git
# repository.

clean:
	rm -rf $(BUILD)

cleaner: clean
