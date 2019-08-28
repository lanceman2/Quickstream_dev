# This is a "GNU make" make file.


# WEB_DOCS_TARGET_DIR directory may be a git clone of
# https://github.com:lanceman2/quickstream.doc.git
#
# It's just so that we can push binary image files to the github.com web
# pages.  It's a different repo then
# https://github.com/lanceman2/quickstream/ so that we don't put generated
# binary image files in the source repo.
#
WEB_DOCS_TARGET_DIR := ../../../quickstream.doc


DOT := $(wildcard *.dot)

WEB_DOCS :=\
 $(patsubst %.dot, $(WEB_DOCS_TARGET_DIR)/%.png, $(DOT))\
 $(patsubst %.dot, $(WEB_DOCS_TARGET_DIR)/%.svg, $(DOT))

IMAGES :=\
 $(patsubst %.dot, images/%.png, $(DOT))\
 $(patsubst %.dot, images/%.svg, $(DOT))

# DOX_TARGET is a file that depends on running doxygen
DOX_TARGET := html/index.html


DOX_INFILES :=\
 ../../include/qsapp.h\
 ../../include/qsfilter.h\
 ../../bin/quickstream.c\
 mainpage.dox

INSTALL_DIR = $(PREFIX)/share/quickstream-doc

# Get a PREFIX if we can from quickbuild config.make
-include ../../config.make

# Get a PREFIX if we can from autotools generated file
ifeq (,$(wildcard prefix.make))
include ../../prefix.make
else
# Get a PREFIX if we can from quickbuild config.make
-include ../../config.make
endif


all: build

build: $(IMAGES) $(DOX_TARGET)


# qsapp.h is a generated file
../../include/qsapp.h: ../../include/qsapp.h.in
	cd ../../include && $(MAKE) qsapp.h


install:
ifdef PREFIX
	mkdir -p $(INSTALL_DIR)
	cp -r html images $(INSTALL_DIR)
else
	@echo "PREFIX is not defined"
endif


Doxyfile: Doxyfile.in
	@echo "# This is a generated file" > $@
	sed $< -e 's!@DOX_INFILES@!$(DOX_INFILES)!g' >> $@

$(DOX_TARGET): Doxyfile $(DOX_INFILES)
	cd html/ && rm -f *.png *.js *.css *.png *.html *.md5 *.map
	rm -rf html/search
	rm -rf latex
	doxygen


# Extra make target that only developers need to know.  It makes binary
# image files for the web that end up in a different repo then the
# quickstream repo, quickstream.doc repo.
#
web_docs: $(WEB_DOCS)



images/%.svg: %.dot
	dot $< -T svg > $@

images/%.png: %.dot
	dot $< -T png > $@


$(WEB_DOCS_TARGET_DIR)/%.png: images/%.png
	cp $< $@

$(WEB_DOCS_TARGET_DIR)/%.svg: images/%.svg
	cp $< $@


show display: $(DOT)
	for f in $^ ; do display $$f & done



clean:
	rm -f $(IMAGES) Doxyfile
	rm -rf latex
	cd html/ && rm -f *.png *.js *.css *.png *.html *.md5 *.map
	rm -rf html/search


cleaner: clean
distclean: cleaner
