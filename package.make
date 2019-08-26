# The quickbuild.make makefile will include this every time make runs.
#
# This is quickstream specific configuration for the quickbuild build
# system.
#
TAR_NAME := quickstream


# Edit to make a release:
#
RELEASE_MAJOR := 0
RELEASE_MINOR := 0
RELEASE_EDIT  := 1

RELEASE_VERSION := $(RELEASE_MAJOR).$(RELEASE_MINOR).$(RELEASE_EDIT)
