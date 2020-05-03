# This is a "GNU make" make file.


topdir := ../../..

QSLIBS := -L$(topdir)/lib -lquickstream -Wl,-rpath=\$$ORIGIN/$(topdir)/lib

# If make was not run in the upper source directories this make file will
# not work correctly.
CHECK_FILES :=\
 $(strip\
 $(topdir)/lib/libquickstream.so\
 $(topdir)/include/quickstream/app.h)


# This ifeq mess brakes the make tab completion.
ifeq (1,0) # So, if this out for now.
# this ifeq stuff makes it so we know why make fails when
# we run make in this directory without building lib/libquickstream and
# include/quickstream/app.h.
ifeq ($(strip $(MAKELEVEL)),0)
# The user is running make from this directory.
ifneq ($(strip $(patsubst %clean, clean, $(patsubst clean%, clean, $(MAKECMDGOALS)))),clean)
# The user is not running 'make clean', 'make cleaner',
# or 'make distclean'.
ifneq ($(strip $(wildcard $(CHECK_FILES))),$(CHECK_FILES))
# The CHECK_FILES are not build in the upper directories.
$(error $(CHECK_FILES) have not been made yet, run 'make -C ../../..')
endif
endif
endif
endif



GEN_DOT := $(patsubst %.dot.bl, %.dot, $(wildcard *.dot.bl))
DOT := $(sort $(wildcard *.dot) $(GEN_DOT))

GEN_IMAGES :=\
 $(sort\
 $(GEN_DOT)\
 $(patsubst %.dot, images/%.png, $(DOT))\
 $(patsubst %.dot, images/%.svg, $(DOT)))

# DOX_TARGET is a file that depends on running doxygen
DOX_TARGET := html/index.html

HTM_FILES := quickstreamIntro.htm quickstreamOptions.htm

QS_DOC_GEN := $(topdir)/lib/quickstream/misc/quickstreamHelp

DOX_INFILES =\
 $(topdir)/include/quickstream/app.h\
 $(topdir)/include/quickstream/filter.h\
 $(topdir)/include/quickstream/parameter.h\
 $(topdir)/include/quickstream/filter.hpp\
 $(topdir)/bin/quickstream.c\
 mainpage.dox




build: $(GEN_DOT) $(GEN_IMAGES) $(DOX_TARGET)


$(DOX_TARGET): $(HTM_FILES)

quickstreamIntro.htm: $(QS_DOC_GEN)
	$< -i > $@

quickstreamOptions.htm: $(QS_DOC_GEN)
	$< -o > $@


# WEB_DOCS_TARGET_DIR directory may be a git clone of the WEB_DOCS_URL
# ../../../RELEASE.bash | grep WEB_DOCS_URL
#
# It's just so that we can push binary image files to the github.com web
# pages.  It's a different repo than
# the one this code is in so that we don't put generated
# files into the quickstream source repository.
#
# Running 'make web_docs' may push changes to the WEB_DOCS_URL
#
# If the directory WEB_DOCS_TARGET_DIR does not exist than we do not
# make the make rules for it.  See ifneq below.

#
WEB_DOCS_TARGET_DIR := $(topdir)/../quickstream_webpages


ifneq ($(wildcard $(WEB_DOCS_TARGET_DIR)/),)
WEB_DOCS :=\
 $(patsubst %.dot, $(WEB_DOCS_TARGET_DIR)/%.png, $(DOT))\
 $(patsubst %.dot, $(WEB_DOCS_TARGET_DIR)/%.svg, $(DOT))

# Extra make target that only developers need to know.  It makes binary
# image files for the web that end up in a different repo then the
# quickstream repo, quickstream.doc repo with branch.
#
# There needs to be a directory that is from something like:
#
#   git clone WEB_DOCS_URL quickstream_webpages
#
#   cd quickstream_webpages
#   git checkout gh-pages
#   cd -
#   make web_docs
#
#   
# there is a script in quickstream_webpages/ that does stuff
# like this:
#
#   git commit -a -m "Adding pages"
#   git push origin gh-pages
#
#
web_docs_make: $(WEB_DOCS)
#
web_docs: web_docs_make $(DOX_TARGET)
	rm -rf $(WEB_DOCS_TARGET_DIR)/html
	cd $(WEB_DOCS_TARGET_DIR)/
	cp -a html $(WEB_DOCS_TARGET_DIR)/
	@echo "\n\n   push pages to the web by running: $(WEB_DOCS_TARGET_DIR)/push_webdocs.bash $(PWD)\n"


$(WEB_DOCS_TARGET_DIR)/%.png: images/%.png
	cp $< $@

$(WEB_DOCS_TARGET_DIR)/%.svg: images/%.svg
	cp $< $@
else
$(info make target web_docs disabled)
endif




# app.h is a generated file because it has a release version number in it.
$(topdir)/include/quickstream/app.h: $(topdir)/include/quickstream/app.h.in
	cd $(topdir)/include/quickstream && $(MAKE) app.h


-include ../../../config.make

INSTALL_DIR = $(PREFIX)/share/doc/quickstream

install:
ifneq ($(PREFIX),)
	mkdir -p $(INSTALL_DIR)
	cp -r html images $(INSTALL_DIR)
else
	@echo "PREFIX is not defined"
endif


Doxyfile: %: %.in
	@echo "# This is a generated file" > $@.tmp
	sed $< -e 's!@DOX_INFILES@!$(DOX_INFILES)!g' >> $@.tmp
	$(topdir)/RELEASE.bash $@.tmp > $@
	rm $@.tmp

mainpage.dox: %: %.in
	$(topdir)/RELEASE.bash $< '//' > $@


$(DOX_TARGET): Doxyfile $(DOX_INFILES)
	cd html/ && rm -f *.png *.js *.css *.png *.html *.md5 *.map
	rm -rf html/search
	rm -rf latex
	doxygen



%.dot: %.dot.bl
	./$<

images/%.svg: %.dot
	dot $< -T svg > $@

images/%.png: %.dot
	dot $< -T png > $@



show display: $(DOT)
	for f in $^ ; do display $$f & done



clean:
	rm -f $(GEN_IMAGES) Doxyfile mainpage.dox Doxyfile.tmp
	rm -f quickstreamIntro.htm quickstreamOptions.htm
	rm -rf latex
	cd html/ && rm -f *.png *.js *.css *.png *.html *.md5 *.map
	rm -rf html/search


cleaner: clean
distclean: cleaner

