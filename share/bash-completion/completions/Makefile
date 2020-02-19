
root := ../../..

INSTALL_DIR = $(PREFIX)/share/bash-completion/completions

IN_VARS := OPTS GEN_FILE

helper := $(root)/lib/quickstream/misc/quickstreamHelp

OPTS := $(shell $(helper) -O)

GEN_FILE := \# This is a generated file

include $(root)/quickbuild.make
