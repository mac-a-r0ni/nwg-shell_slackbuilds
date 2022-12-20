dnl Copyright (C) 2008 Kim Woelders
dnl This code is public domain and can be freely used or copied.

dnl Macro to set compiler warning flags in CFLAGS_WARNINGS

dnl Provides configure argument --enable-werror to stop compilation on warnings

dnl Usage: EC_C_WARNINGS([LANG])
dnl Set LANG to 'cpp' when compiling for C++

AC_DEFUN([EC_C_WARNINGS], [
  define(ec_c_compile_cpp, ifelse([$1], [cpp], [yes], [no]))

  AC_ARG_ENABLE(werror,
    [  --enable-werror         treat compiler warnings as errors @<:@default=no@:>@],,
    enable_werror=no)

  if test "x$GCC" = "xyes"; then
    CFLAGS_WARNINGS="-W -Wall -Wcast-align -Wpointer-arith -Wshadow -Wwrite-strings"
dnl # ignore some warnings for now...
    CFLAGS_WARNINGS="$CFLAGS_WARNINGS -Wno-unused-parameter"
    ifelse(ec_c_compile_cpp, no, [
      CFLAGS_WARNINGS="$CFLAGS_WARNINGS -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes"
    ],)

    if test "x$enable_werror" = "xyes"; then
      CFLAGS_WARNINGS="$CFLAGS_WARNINGS -Werror"
    fi
  fi
  AC_SUBST(CFLAGS_WARNINGS)
])
