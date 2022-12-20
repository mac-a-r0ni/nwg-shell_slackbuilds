EXTRA_DIST = Doxyfile.in imlib2_intro.dox.in imlib2_logo.png

DEPFILES = Doxyfile imlib2_intro.dox

all-local: html/index.html

html/index.html: $(DEPFILES) ../src/lib/Imlib2.h
	rm -rf html man
	doxygen

clean-local:
	rm -rf $(DEPFILES) html man
