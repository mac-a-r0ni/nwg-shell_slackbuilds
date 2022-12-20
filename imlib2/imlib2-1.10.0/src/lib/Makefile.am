AUTOMAKE_OPTIONS     = 1.4 foreign
AM_CFLAGS            = $(CFLAGS_WARNINGS) $(CFLAGS_VISIBILITY) $(CFLAGS_ASAN)
AM_CPPFLAGS          = -DPACKAGE_LIB_DIR=\"$(libdir)\" \
                       -I$(top_builddir) \
                       $(X_CFLAGS) \
                       $(FREETYPE_CFLAGS)

# Older automakes (<1.10) do not use INCLUDES when compiling .S files
AM_CCASFLAGS = -I$(top_builddir)

lib_LTLIBRARIES      = libImlib2.la
include_HEADERS      = Imlib2.h Imlib2_Loader.h

libImlib2_la_LIBADD  =

libImlib2_la_SOURCES = \
api.c 		api.h		\
api_obsolete.c	\
asm.h \
asm_c.c		asm_c.h		\
blend.c		blend.h		\
color_helpers.c	color_helpers.h \
colormod.c	colormod.h	\
common.h \
debug.c		debug.h		\
draw_ellipse.c \
draw_line.c \
draw_polygon.c \
draw_rectangle.c \
file.c		file.h		\
grad.c		grad.h		\
image.c		image.h		\
image_tags.c \
loaders.c	loaders.h	\
modules.c \
object.c	object.h	\
rgbadraw.c	rgbadraw.h	\
rotate.c	rotate.h	\
scale.c		scale.h		\
span.c		span.h		\
strutils.c	strutils.h	\
types.h	\
updates.c	updates.h
if ENABLE_FILTERS
libImlib2_la_SOURCES += \
api_filter.c	\
dynamic_filters.c	dynamic_filters.h	\
filter.c	filter.h	\
script.c	script.h
endif
if ENABLE_TEXT
libImlib2_la_SOURCES += \
api_text.c	\
font.h \
font_draw.c \
font_load.c \
font_main.c \
font_query.c
libImlib2_la_LIBADD += $(FREETYPE_LIBS)
endif

MMX_SRCS = \
asm_blend.S \
asm_blend_cmod.S \
asm_rgba.S \
asm_rotate.S \
asm_scale.S

AMD64_SRCS = \
amd64_blend.S \
amd64_blend_cmod.S

EXTRA_DIST = $(MMX_SRCS) $(AMD64_SRCS) asm_loadimmq.S

if BUILD_X11
libImlib2_la_SOURCES += \
api_x11.c	\
x11_color.c	x11_color.h	\
x11_context.c	x11_context.h	\
x11_grab.c	x11_grab.h	\
x11_pixmap.c	x11_pixmap.h	\
x11_rend.c	x11_rend.h	\
x11_rgba.c	x11_rgba.h	\
x11_ximage.c	x11_ximage.h
libImlib2_la_LIBADD += -lXext -lX11 $(X_SHM_FD_LIBS)
endif
if BUILD_DEBUG
libImlib2_la_LIBADD += $(CLOCK_LIBS)
endif

if BUILD_MMX
libImlib2_la_SOURCES += $(MMX_SRCS)
endif
if BUILD_AMD64
libImlib2_la_SOURCES += $(AMD64_SRCS)
endif

libImlib2_la_LIBADD += $(DLOPEN_LIBS) -lm
libImlib2_la_LDFLAGS = -version-info @lt_version@
