
SUBDIRS :=\
 include\
 lib\
 bin\
 lib/quickstream/plugins/filters\
 share/quickstream-doc

quickbuild.make:
	./quickbuild

include quickbuild.make
