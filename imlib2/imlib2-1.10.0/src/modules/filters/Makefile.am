AM_CFLAGS            = $(CFLAGS_WARNINGS) $(CFLAGS_VISIBILITY) $(CFLAGS_ASAN)
AM_CPPFLAGS          = -I$(top_builddir) -I$(top_builddir)/src/lib -I$(top_srcdir)/src/lib

pkgdir                = $(libdir)/imlib2/filters
pkg_LTLIBRARIES       = testfilter.la bumpmap.la colormod.la

EXTRA_DIST = filter_common.h

testfilter_la_SOURCES = filter_test.c
testfilter_la_LDFLAGS = -module -avoid-version
testfilter_la_LIBADD  = $(top_builddir)/src/lib/libImlib2.la
testfilter_la_LIBTOOLFLAGS = --tag=disable-static

bumpmap_la_SOURCES    = filter_bumpmap.c
bumpmap_la_LDFLAGS    = -module -avoid-version
bumpmap_la_LIBADD     = $(top_builddir)/src/lib/libImlib2.la -lm
bumpmap_la_LIBTOOLFLAGS = --tag=disable-static

colormod_la_SOURCES   = filter_colormod.c
colormod_la_LDFLAGS   = -module -avoid-version
colormod_la_LIBADD    = $(top_builddir)/src/lib/libImlib2.la
colormod_la_LIBTOOLFLAGS = --tag=disable-static

