
SUBDIRS := include lib bin lib/plugins/filters share/quickstream-doc

quickbuild.make:
	./quickbuild

include quickbuild.make
