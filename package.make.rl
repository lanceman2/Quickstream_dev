# The quickbuild.make makefile will include this every time make runs.
#
# This is quickstream specific configuration for the quickbuild build
# system.  This should not contain any user installation configuration.
#

# quickbuild.make looks are TAR_NAME when composing the default
# prefix.
TAR_NAME := @PACKAGE_NAME@


PACKAGE_VERSION := @PACKAGE_VERSION@
