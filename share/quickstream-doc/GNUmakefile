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


build: $(IMAGES) html/index.html



html/index.html:\
 ../../include/qsapp.h\
 ../../include/qsfilter.h\
 Doxyfile\
 quickstream.dox
	cd html/ && rm -f *.png *.js *.css *.png *.html *.md5
	rm -rf html/search
	rm -rf latex
	doxygen


# Extra make target that only developers need to know:
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
	rm -f $(IMAGES)
	rm -rf latex
	cd html/ && rm -f *.png *.js *.css *.png *.html *.md5
	rm -rf html/search


cleaner: clean
