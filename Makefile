# This file is for building and installing quickstream with quickbuild,
# not GNU autotools.  You can build and install quickstream with GNU
# autotools.
#
# The make files with name 'makefile' are generated from 'makefile.am'
# using GNU autotools, specifically GNU automake; and we used the make
# file name 'Makefile' and sometimes 'GNUmakefile' for using quickbuild.

SUBDIRS :=\
 include/quickstream\
 lib\
 lib/quickstream/misc\
 bin\
 lib/quickstream/plugins/filters\
 lib/quickstream/plugins/controllers\
 share/bash-completion/completions\
 share/doc/quickstream


ifeq ($(strip $(subst cleaner, clean, $(MAKECMDGOALS))),clean)
SUBDIRS +=\
 dev_tests\
 tests\
 tests/interactive_tests
endif


ifneq ($(wildcard package.make),package.make)
$(error "First run './quickbuild'")
endif

ifneq ($(wildcard quickbuild.make),quickbuild.make)
$(error "First run './quickbuild'")
endif


include quickbuild.make
