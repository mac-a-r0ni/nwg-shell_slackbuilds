AM_CFLAGS            = $(CFLAGS_WARNINGS) $(CFLAGS_VISIBILITY) $(CFLAGS_ASAN)
AM_CPPFLAGS          = -I$(top_builddir) -I$(top_srcdir)/src/lib

pkgdir               = $(libdir)/imlib2/loaders

pkg_LTLIBRARIES = \
ani.la \
argb.la \
bmp.la \
ff.la \
ico.la \
lbm.la \
pnm.la \
tga.la \
xbm.la \
xpm.la

if BUILD_GIF_LOADER
pkg_LTLIBRARIES += gif.la
endif
if BUILD_HEIF_LOADER
pkg_LTLIBRARIES += heif.la
endif
if BUILD_JPEG_LOADER
pkg_LTLIBRARIES += jpeg.la
endif
if BUILD_J2K_LOADER
pkg_LTLIBRARIES += j2k.la
endif
if BUILD_JXL_LOADER
pkg_LTLIBRARIES += jxl.la
endif
if BUILD_PNG_LOADER
pkg_LTLIBRARIES += png.la
endif
if BUILD_PS_LOADER
pkg_LTLIBRARIES += ps.la
endif
if BUILD_SVG_LOADER
pkg_LTLIBRARIES += svg.la
endif
if BUILD_TIFF_LOADER
pkg_LTLIBRARIES += tiff.la
endif
if BUILD_WEBP_LOADER
pkg_LTLIBRARIES += webp.la
endif

if BUILD_BZ2_LOADER
pkg_LTLIBRARIES += bz2.la
endif
if BUILD_LZMA_LOADER
pkg_LTLIBRARIES += lzma.la
endif
if BUILD_ZLIB_LOADER
pkg_LTLIBRARIES += zlib.la
endif

if BUILD_ID3_LOADER
pkg_LTLIBRARIES += id3.la
endif

ani_la_SOURCES       = loader_ani.c
ani_la_LDFLAGS       = -module -avoid-version
ani_la_LIBADD        = $(top_builddir)/src/lib/libImlib2.la
ani_la_LIBTOOLFLAGS  = --tag=disable-static

argb_la_SOURCES      = loader_argb.c
argb_la_LDFLAGS      = -module -avoid-version
argb_la_LIBADD       = $(top_builddir)/src/lib/libImlib2.la
argb_la_LIBTOOLFLAGS = --tag=disable-static

bmp_la_SOURCES       = loader_bmp.c
bmp_la_LDFLAGS       = -module -avoid-version
bmp_la_LIBADD        = $(top_builddir)/src/lib/libImlib2.la
bmp_la_LIBTOOLFLAGS  = --tag=disable-static

ff_la_SOURCES        = loader_ff.c
ff_la_LDFLAGS        = -module -avoid-version
ff_la_LIBADD         = $(top_builddir)/src/lib/libImlib2.la
ff_la_LIBTOOLFLAGS   = --tag=disable-static

gif_la_SOURCES       = loader_gif.c
gif_la_LDFLAGS       = -module -avoid-version
gif_la_LIBADD        = $(GIF_LIBS) $(top_builddir)/src/lib/libImlib2.la
gif_la_LIBTOOLFLAGS  = --tag=disable-static

heif_la_SOURCES      = loader_heif.c
heif_la_CPPFLAGS     = $(HEIF_CFLAGS) $(AM_CPPFLAGS)
heif_la_LDFLAGS      = -module -avoid-version -Wl,-z,nodelete
heif_la_LIBADD       = $(HEIF_LIBS) $(top_builddir)/src/lib/libImlib2.la
heif_la_LIBTOOLFLAGS = --tag=disable-static

ico_la_SOURCES       = loader_ico.c
ico_la_LDFLAGS       = -module -avoid-version
ico_la_LIBADD        = $(top_builddir)/src/lib/libImlib2.la
ico_la_LIBTOOLFLAGS  = --tag=disable-static

jpeg_la_SOURCES      = loader_jpeg.c exif.c exif.h
jpeg_la_CPPFLAGS     = $(JPEG_CFLAGS) $(AM_CPPFLAGS)
jpeg_la_LDFLAGS      = -module -avoid-version
jpeg_la_LIBADD       = $(JPEG_LIBS) $(top_builddir)/src/lib/libImlib2.la
jpeg_la_LIBTOOLFLAGS = --tag=disable-static

j2k_la_SOURCES       = loader_j2k.c
j2k_la_CPPFLAGS      = $(J2K_CFLAGS) $(AM_CPPFLAGS)
j2k_la_LDFLAGS       = -module -avoid-version
j2k_la_LIBADD        = $(J2K_LIBS) $(top_builddir)/src/lib/libImlib2.la
j2k_la_LIBTOOLFLAGS  = --tag=disable-static

jxl_la_SOURCES       = loader_jxl.c
jxl_la_CPPFLAGS      = $(JXL_CFLAGS) $(AM_CPPFLAGS)
jxl_la_LDFLAGS       = -module -avoid-version
jxl_la_LIBADD        = $(JXL_LIBS) $(top_builddir)/src/lib/libImlib2.la
jxl_la_LIBTOOLFLAGS  = --tag=disable-static

lbm_la_SOURCES       = loader_lbm.c
lbm_la_LDFLAGS       = -module -avoid-version
lbm_la_LIBADD        = $(top_builddir)/src/lib/libImlib2.la
lbm_la_LIBTOOLFLAGS  = --tag=disable-static

png_la_SOURCES       = loader_png.c
png_la_CPPFLAGS      = $(PNG_CFLAGS) $(AM_CPPFLAGS)
png_la_LDFLAGS       = -module -avoid-version
png_la_LIBADD        = $(PNG_LIBS) $(top_builddir)/src/lib/libImlib2.la
png_la_LIBTOOLFLAGS  = --tag=disable-static

pnm_la_SOURCES       = loader_pnm.c
pnm_la_LDFLAGS       = -module -avoid-version
pnm_la_LIBADD        = $(top_builddir)/src/lib/libImlib2.la
pnm_la_LIBTOOLFLAGS  = --tag=disable-static

ps_la_SOURCES       = loader_ps.c
ps_la_CPPFLAGS      = $(PS_CFLAGS) $(AM_CPPFLAGS)
ps_la_LDFLAGS       = -module -avoid-version
ps_la_LIBADD        = $(PS_LIBS) $(top_builddir)/src/lib/libImlib2.la
ps_la_LIBTOOLFLAGS  = --tag=disable-static

svg_la_SOURCES       = loader_svg.c
svg_la_CPPFLAGS      = $(SVG_CFLAGS) $(AM_CPPFLAGS)
svg_la_LDFLAGS       = -module -avoid-version -Wl,-z,nodelete
svg_la_LIBADD        = $(SVG_LIBS) $(top_builddir)/src/lib/libImlib2.la
svg_la_LIBTOOLFLAGS  = --tag=disable-static

tga_la_SOURCES       = loader_tga.c
tga_la_LDFLAGS       = -module -avoid-version
tga_la_LIBADD        = $(top_builddir)/src/lib/libImlib2.la
tga_la_LIBTOOLFLAGS  = --tag=disable-static

tiff_la_SOURCES      = loader_tiff.c
tiff_la_CPPFLAGS     = $(TIFF_CFLAGS) $(AM_CPPFLAGS)
tiff_la_LDFLAGS      = -module -avoid-version
tiff_la_LIBADD       = $(TIFF_LIBS) $(top_builddir)/src/lib/libImlib2.la
tiff_la_LIBTOOLFLAGS = --tag=disable-static

webp_la_SOURCES      = loader_webp.c
webp_la_CPPFLAGS     = $(WEBP_CFLAGS) $(AM_CPPFLAGS)
webp_la_LDFLAGS      = -module -avoid-version
webp_la_LIBADD       = $(WEBP_LIBS) $(top_builddir)/src/lib/libImlib2.la
webp_la_LIBTOOLFLAGS = --tag=disable-static

xbm_la_SOURCES       = loader_xbm.c
xbm_la_LDFLAGS       = -module -avoid-version
xbm_la_LIBADD        = $(top_builddir)/src/lib/libImlib2.la
xbm_la_LIBTOOLFLAGS  = --tag=disable-static

xpm_la_SOURCES       = loader_xpm.c
xpm_la_LDFLAGS       = -module -avoid-version
xpm_la_LIBADD        = $(top_builddir)/src/lib/libImlib2.la
xpm_la_LIBTOOLFLAGS  = --tag=disable-static

bz2_la_SOURCES       = loader_bz2.c decompress_load.c
bz2_la_CPPFLAGS      = $(BZ2_CFLAGS) $(AM_CPPFLAGS)
bz2_la_LDFLAGS       = -module -avoid-version
bz2_la_LIBADD        = $(BZ2_LIBS) $(top_builddir)/src/lib/libImlib2.la
bz2_la_LIBTOOLFLAGS  = --tag=disable-static

lzma_la_SOURCES      = loader_lzma.c decompress_load.c
lzma_la_CPPFLAGS     = $(LZMA_CFLAGS) $(AM_CPPFLAGS)
lzma_la_LDFLAGS      = -module -avoid-version
lzma_la_LIBADD       = $(LZMA_LIBS) $(top_builddir)/src/lib/libImlib2.la
lzma_la_LIBTOOLFLAGS = --tag=disable-static

zlib_la_SOURCES      = loader_zlib.c decompress_load.c
zlib_la_CPPFLAGS     = $(ZLIB_CFLAGS) $(AM_CPPFLAGS)
zlib_la_LDFLAGS      = -module -avoid-version
zlib_la_LIBADD       = $(ZLIB_LIBS) $(top_builddir)/src/lib/libImlib2.la
zlib_la_LIBTOOLFLAGS = --tag=disable-static

id3_la_SOURCES       = loader_id3.c
id3_la_CPPFLAGS      = $(ID3_CFLAGS) $(AM_CPPFLAGS)
id3_la_LDFLAGS       = -module -avoid-version
id3_la_LIBADD        = $(ID3_LIBS) $(top_builddir)/src/lib/libImlib2.la
id3_la_LIBTOOLFLAGS  = --tag=disable-static
