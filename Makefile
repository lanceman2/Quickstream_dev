
SUBDIRS :=\
 include/quickstream\
 lib\
 bin\
 lib/quickstream/plugins/filters\
 share/doc/quickstream

ifeq ($(strip $(subst cleaner, clean, $(MAKECMDGOALS))),clean)
SUBDIRS +=\
 dev_tests\
 tests\
 tests/interactive_tests
endif

quickbuild.make: 
	./quickbuild

include quickbuild.make
