
SUBDIRS :=\
 include\
 lib\
 bin\
 lib/quickstream/plugins/filters\
 share/doc/quickstream

quickbuild.make: 
	./quickbuild

include quickbuild.make
