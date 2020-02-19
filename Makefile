
SUBDIRS :=\
 include/quickstream\
 lib\
 lib/quickstream/misc\
 bin\
 lib/quickstream/plugins/filters\
 share/doc/quickstream\
 share/bash-completion/completions


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
