
root := ../../..

INSTALL_DIR = $(PREFIX)/share/bash-completion/completions

IN_VARS := OPTS GEN_FILE OPTS_NOARG

helper := $(root)/lib/quickstream/misc/quickstreamHelp

OPTS := $(shell $(helper) -O)
OPTS_NOARG := $(shell $(helper) -w)

GEN_FILE := \# This is a generated file

include $(root)/quickbuild.make
