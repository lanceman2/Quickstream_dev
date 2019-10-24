
SUBDIRS :=\
 include\
 lib\
 bin\
 lib/quickstream/plugins/filters\
 share/doc/quickstream\
 dev_tests\
 tests

quickbuild.make: 
	./quickbuild

include quickbuild.make
