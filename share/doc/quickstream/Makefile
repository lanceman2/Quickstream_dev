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
# we run make in this directory without building libquickstream and
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



BUILD_NO_INSTALL := quickstream_complex_dot quickstream_simple_dot
quickstream_complex_dot_LDADD := $(QSLIBS)
quickstream_simple_dot_LDADD := $(QSLIBS)


# WEB_DOCS_TARGET_DIR directory may be a git clone of
# https://github.com:lanceman2/quickstream.doc.git or
# git@github.com:lanceman2/quickstream.doc
#
# It's just so that we can push binary image files to the github.com web
# pages.  It's a different repo then
# https://github.com/lanceman2/quickstream/ so that we don't put generated
# binary image files in the source repo.
#
WEB_DOCS_TARGET_DIR := $(topdir)/../quickstream.doc


DOT := $(sort\
 $(wildcard *.dot)\
 quickstream_complex.dot\
 quickstream_simple.dot)


WEB_DOCS :=\
 $(patsubst %.dot, $(WEB_DOCS_TARGET_DIR)/%.png, $(DOT))\
 $(patsubst %.dot, $(WEB_DOCS_TARGET_DIR)/%.svg, $(DOT))



IMAGES :=\
 $(sort\
 quickstream_complex.dot\
 quickstream_simple.dot\
 $(patsubst %.dot, images/%.png, $(DOT))\
 $(patsubst %.dot, images/%.svg, $(DOT)))

# DOX_TARGET is a file that depends on running doxygen
DOX_TARGET := html/index.html

DOX_INFILES =\
 $(topdir)/include/quickstream/app.h\
 $(topdir)/include/quickstream/filter.h\
 $(topdir)/bin/quickstream.c\
 mainpage.dox

INSTALL_DIR = $(PREFIX)/share/doc/quickstream




-include ../../config.make


build: $(IMAGES) $(DOX_TARGET)


%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_NO_INSTALL): %:%.o
	$(CC) $(CFLAGS) -o $@ $< $($@_LDADD)

quickstream_complex.dot: quickstream_complex_dot
	./$< > $@
quickstream_simple.dot: quickstream_simple_dot
	./$< > $@


# app.h is a generated file
$(topdir)/include/quickstream/app.h: $(topdir)/include/quickstream/app.h.in
	cd $(topdir)/include/quickstream && $(MAKE) app.h


install:
ifdef PREFIX
	mkdir -p $(INSTALL_DIR)
	cp -r html images $(INSTALL_DIR)
else
	@echo "PREFIX is not defined"
endif


Doxyfile: Doxyfile.in
	@echo "# This is a generated file" > $@.tmp
	sed $< -e 's!@DOX_INFILES@!$(DOX_INFILES)!g' >> $@.tmp
	$(topdir)/RELEASE.bash $@.tmp > $@
	rm $@.tmp


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
	rm -f $(IMAGES) Doxyfile Doxyfile.tmp $(BUILD_NO_INSTALL) *.o
	rm -rf latex
	cd html/ && rm -f *.png *.js *.css *.png *.html *.md5 *.map
	rm -rf html/search


cleaner: clean
distclean: cleaner

