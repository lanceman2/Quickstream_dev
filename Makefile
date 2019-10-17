
SUBDIRS :=\
 include\
 lib\
 bin\
 lib/quickstream/plugins/filters\
 share/doc/quickstream\
 dev_tests

quickbuild.make: 
	./quickbuild

include quickbuild.make
