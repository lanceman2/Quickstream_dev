# The quickbuild.make Makefile will include this every time make runs.
#
# This is quickstream specific configuration for the quickbuild build
# system.  This should not contain any user installation configuration.
# 
# This file should not have configuration information for a particular
# installation.
#

# quickbuild.make looks at TAR_NAME when composing the default
# prefix.
TAR_NAME := @PACKAGE_NAME@

PACKAGE_VERSION := @PACKAGE_VERSION@

PACKAGE_URL := @PACKAGE_URL@

