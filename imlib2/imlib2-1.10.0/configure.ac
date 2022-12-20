AC_INIT([imlib2],[1.10.0],[enlightenment-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR(configure.ac)
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE(1.6 dist-xz -Wno-portability)
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_USE_SYSTEM_EXTENSIONS
AC_C_BIGENDIAN
AC_PROG_CC
AM_PROG_AS
AC_PROG_CXX

AC_C_CONST

dnl Next two lines is a hack to prevent libtool checking for CXX/F77
define([AC_LIBTOOL_LANG_CXX_CONFIG], [:])dnl
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])dnl
LT_INIT

VERSION_CUR=11
VERSION_REV=0
VERSION_AGE=10
lt_version=${VERSION_CUR}:${VERSION_REV}:${VERSION_AGE}
AC_SUBST(lt_version)

PKG_PROG_PKG_CONFIG

AC_DEFUN([EC_LOADER_CHECK], [
  m4_pushdef([NMDN], m4_tolower($1))
  m4_pushdef([NMUP], m4_toupper($1))
  m4_pushdef([DFLT], m4_strip($2))
  AC_MSG_CHECKING([whether to enable NMUP support])
  AC_ARG_WITH([NMDN],
    AS_HELP_STRING([--without-NMDN], [Disable NMUP loader]),
    [ NMDN[]_loader="$withval" ], [ NMDN[]_loader="DFLT" ]
  )
  AC_MSG_RESULT($NMDN[]_loader)

  if test "$NMDN[]_loader" != "no"; then
    ifelse([$3], [], [
      NMDN[]_ok="no"
    ], [
      PKG_CHECK_MODULES(NMUP, $3, [ NMDN[]_ok="yes" ], [ NMDN[]_ok="no" ])
    ])
    ifelse([$4], [],, [
      if test "$NMDN[]_ok" = "no" ; then
        $4
      fi
    ])
    if test "$NMDN[]_ok" = "no" ; then
      if test "$NMDN[]_loader" = "yes" ; then
        AC_MSG_ERROR([NMUP support was requested but system does not support it])
      else
        AC_MSG_WARN([*** NMUP support will not be built ($2 not found) ***])
      fi
    fi
  else
    NMDN[]_ok="no"
  fi

  if test "$NMDN[]_ok" = "yes" ; then
    AC_DEFINE(BUILD_[]NMUP[]_LOADER, [ 1 ], [ Build NMUP loader ])
  fi
  AM_CONDITIONAL(BUILD_[]NMUP[]_LOADER, [ test "$NMDN[]_ok" = "yes" ])

  m4_popdef([NMDN])
  m4_popdef([NMUP])
])


mmx=no
amd64=no
case $host_cpu in
  i*86)   mmx="yes";;
  x86_64) amd64="yes";;
  amd64) amd64="yes";;
esac

case $host_os in
  solaris*) mmx="no";;
  darwin*) mmx="no";;
esac

AC_ARG_ENABLE([mmx],
  [AS_HELP_STRING([--enable-mmx], [attempt compiling using x86 mmx assembly @<:@default=auto@:>@])],
  [
   if test x$enableval = xyes; then
     mmx=yes
     # Cannot compile with both options enabled
     amd64=no
   else
     mmx=no
   fi
  ]
)

AC_ARG_ENABLE([amd64],
  [AS_HELP_STRING([--enable-amd64], [attempt compiling using amd64 assembly @<:@default=auto@:>@])],
  [
   if test x$enableval = xyes; then
     amd64=yes
     mmx=no
   else
     amd64=no
   fi
  ]
)

AC_MSG_CHECKING(whether to enable x86 mmx support)
if test x$mmx = xyes; then
  AC_DEFINE(DO_MMX_ASM, 1, [enabling MMX Assembly])
fi
AC_MSG_RESULT($mmx)
AM_CONDITIONAL(BUILD_MMX, test x$mmx = xyes)

AC_MSG_CHECKING(whether to enable amd64 asm support)
if test x$amd64 = xyes; then
  AC_DEFINE(DO_AMD64_ASM, 1, [enabling AMD64 Assembly])
fi
AC_MSG_RESULT($amd64)
AM_CONDITIONAL(BUILD_AMD64, test x$amd64 = xyes)

# check for freetype
PKG_CHECK_MODULES(FREETYPE, freetype2)

AC_PATH_X
AC_PATH_XTRA

if test "x$have_x" = "xno"; then
   # If --without-x was passed, this will be "disabled" instead of "no" --
   # so always treat "no" as an error
   AC_MSG_ERROR(
[no X support found.  Use --x-includes and --x-libraries to specify the X
path on your system, or --without-x to disable X support.])
fi

if test "x$have_x" = "xyes"; then
  AC_DEFINE(BUILD_X11, 1, [enabling X11 support])
  AC_MSG_CHECKING(whether to enable X11 MIT-SHM FD-passing support)
  AC_ARG_WITH([x-shm-fd],
    [AS_HELP_STRING([--without-x-shm-fd], [Disable X11 MIT-SHM FD-passing support])],
    [
     if test "$withval" = no ; then
       x_shm_fd=no
     else
       x_shm_fd=yes
     fi
    ],
    [ x_shm_fd=auto ]
  )
  AC_MSG_RESULT($x_shm_fd)

  if test "$x_shm_fd" != no ; then
    PKG_CHECK_MODULES(X_SHM_FD, x11-xcb xcb xcb-shm >= 1.9, [ x_shm_fd="yes" ], [ x_shm_fd="no"])
  else
    x_shm_fd="no"
  fi
  if test "$x_shm_fd" = yes ; then
    AC_DEFINE(HAVE_X11_SHM_FD, 1, [enabling X11 MIT-SHM FD-passing support])
  fi
else
  have_x="no"
  x_shm_fd="no"
fi
AM_CONDITIONAL(BUILD_X11, test "x$have_x" = "xyes")

AC_CHECK_LIB(dl, dlopen, DLOPEN_LIBS=-ldl)
AC_SUBST(DLOPEN_LIBS)

AC_CHECK_FUNCS([clock_gettime], [have_clock_gettime=yes],
  [AC_CHECK_LIB([rt], [clock_gettime], [have_clock_gettime=-lrt],
     [have_clock_gettime=no])])
if test "x$have_clock_gettime" != xno; then
  AC_DEFINE(USE_MONOTONIC_CLOCK, 1, [Use monotonic clock from clock_gettime()])
  if test "x$have_clock_gettime" != xyes; then
    AC_SUBST(CLOCK_LIBS, [$have_clock_gettime])
  fi
fi


loader_check_gif() {
  AC_CHECK_LIB(gif, DGifOpenFileName, gif_libs="-lgif" gif_ok=yes, gif_ok=no)
  if test "$gif_ok" = yes; then
    AC_CHECK_HEADER([gif_lib.h], , gif_ok=no)
    if test "$gif_ok" = yes; then
      GIF_LIBS=$gif_libs
      AC_SUBST(GIF_LIBS)
    fi
  fi
}

EC_LOADER_CHECK(GIF,  auto, , loader_check_gif)
EC_LOADER_CHECK(HEIF, auto, libheif)
EC_LOADER_CHECK(JPEG, auto, libjpeg)
EC_LOADER_CHECK(J2K,  auto, libopenjp2)
EC_LOADER_CHECK(JXL,  auto, libjxl libjxl_threads)
EC_LOADER_CHECK(PNG,  auto, libpng)
EC_LOADER_CHECK(PS,   auto, libspectre)
EC_LOADER_CHECK(SVG,  auto, librsvg-2.0 >= 2.46)
EC_LOADER_CHECK(TIFF, auto, libtiff-4)
EC_LOADER_CHECK(WEBP, auto, libwebpdemux)

loader_check_bz2() {
  AC_CHECK_LIB(bz2, BZ2_bzRead, bz2_ok=yes, bz2_ok=no)
  if test "$bz2_ok" = yes; then
    AC_CHECK_HEADER([bzlib.h], , bz2_ok=no)
    if test "$bz2_ok" = yes; then
      BZ2_LIBS="-lbz2"
    fi
  fi
}

EC_LOADER_CHECK(BZ2,  auto, bzip2, loader_check_bz2)
EC_LOADER_CHECK(LZMA, auto, liblzma)
EC_LOADER_CHECK(ZLIB, auto, zlib)

EC_LOADER_CHECK(ID3,  auto, id3tag)


AM_CONDITIONAL(BUILD_TEST, false)

AC_ARG_ENABLE([filters],
  [AS_HELP_STRING([--enable-filters], [Enable filters @<:@default=yes@:>@])],
  enable_filters="$enableval",
  enable_filters="yes"
)
if test "$enable_filters" = "yes"; then
  AC_DEFINE(ENABLE_FILTERS, 1, [Enable filters])
fi
AM_CONDITIONAL(ENABLE_FILTERS, test "$enable_filters" = "yes")

AC_ARG_ENABLE([text],
  [AS_HELP_STRING([--enable-text], [Enable text (TrueType font functions) @<:@default=yes@:>@])],
  enable_text="$enableval",
  enable_text="yes"
)
if test "$enable_text" = "yes"; then
  AC_DEFINE(ENABLE_TEXT, 1, [Enable text functions])
fi
AM_CONDITIONAL(ENABLE_TEXT, test "$enable_text" = "yes")

AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug], [Enable debug features @<:@default=no@:>@])],
  [
    if test x$enableval = xyes; then
      AC_DEFINE(IMLIB2_DEBUG, 1, [Enable debug features])
    fi
  ]
)
AM_CONDITIONAL(BUILD_DEBUG, test "$enable_debug" = "yes")

AC_ARG_ENABLE([doc-build],
  [AS_HELP_STRING([--enable-doc-build], [Enable building documentation (not installed) @<:@default=no@:>@])]
)
AM_CONDITIONAL(BUILD_DOC, test "$enable_doc_build" = "yes")

EC_C_WARNINGS()
EC_C_VISIBILITY(yes)
EC_C_ASAN()

VERSION_MAJOR=`echo $VERSION | awk -F. '{print $1}'`
VERSION_MINOR=`echo $VERSION | awk -F. '{print $2}'`
VERSION_MICRO=`echo $VERSION | awk -F. '{print $3}'`
AC_SUBST(VERSION_MAJOR)
AC_SUBST(VERSION_MINOR)
AC_SUBST(VERSION_MICRO)

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
Makefile
imlib2.pc
src/Makefile
src/lib/Makefile
src/lib/Imlib2.h
src/bin/Makefile
src/modules/Makefile
src/modules/filters/Makefile
src/modules/loaders/Makefile
test/Makefile
data/Makefile
data/fonts/Makefile
data/images/Makefile
doc/Makefile
doc/Doxyfile
doc/imlib2_intro.dox
README
])
AC_OUTPUT

#####################################################################
## Info
echo
echo
echo "------------------------------------------------------------------------"
echo "$PACKAGE $VERSION"
echo "------------------------------------------------------------------------"
echo
echo "Configuration Options Summary:"
echo
echo "Image loaders:"
echo " Regular image loaders"
echo "  GIF.....................: $gif_ok"
echo "  HEIF....................: $heif_ok"
echo "  JPEG....................: $jpeg_ok"
echo "  J2K.....................: $j2k_ok"
echo "  JXL.....................: $jxl_ok"
echo "  PNG.....................: $png_ok"
echo "  PS......................: $ps_ok"
echo "  SVG.....................: $svg_ok"
echo "  TIFF....................: $tiff_ok"
echo "  WEBP....................: $webp_ok"
echo " Decompressors"
echo "  BZIP2...................: $bz2_ok"
echo "  LZMA(XZ)................: $lzma_ok"
echo "  ZLIB....................: $zlib_ok"
echo " Containers"
echo "  ID3.....................: $id3_ok"
echo
echo "Build for X11.............: $have_x"
echo "Use X MIT-SHM FD-passing..: $x_shm_fd"
echo
echo "Use X86 MMX for speed.....: $mmx"
echo "Use AMD64 for speed.......: $amd64"
echo
echo "Include filters...........: $enable_filters"
echo "Include text functions....: $enable_text"
echo "Use visibility hiding.....: $enable_visibility_hiding"
echo
echo "Installation path.........: $prefix"
echo
