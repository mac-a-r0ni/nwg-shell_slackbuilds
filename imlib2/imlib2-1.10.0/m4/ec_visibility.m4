# visibility.m4 serial 1 (gettext-0.15)
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Bruno Haible.

dnl Tests whether the compiler supports the command-line option
dnl -fvisibility=hidden and the function and variable attributes
dnl __attribute__((__visibility__("hidden"))) and
dnl __attribute__((__visibility__("default"))).
dnl Does *not* test for __visibility__("protected") - which has tricky
dnl semantics (see the 'vismain' test in glibc) and does not exist e.g. on
dnl MacOS X.
dnl Does *not* test for __visibility__("internal") - which has processor
dnl dependent semantics.
dnl Does *not* test for #pragma GCC visibility push(hidden) - which is
dnl "really only recommended for legacy code".
dnl Set the variable CFLAGS_VISIBILITY.
dnl Defines and sets the variable HAVE_VISIBILITY.

dnl Modified for E stuff by Kim Woelders

AC_DEFUN([EC_C_VISIBILITY],
[
  AC_REQUIRE([AC_PROG_CC])

  define(ec_c_vis_default, ifelse([$1], [no], [no], [yes]))
  CFLAGS_VISIBILITY=
  HAVE_VISIBILITY=0

  AC_ARG_ENABLE([visibility-hiding],
    [AS_HELP_STRING([--enable-visibility-hiding],
                    [enable visibility hiding @<:@default=]ec_c_vis_default[@:>@])],,
    [enable_visibility_hiding=]ec_c_vis_default)

  if test -n "$GCC" -a "x$enable_visibility_hiding" = "xyes"; then
    AC_MSG_CHECKING([for simple visibility declarations])
    AC_CACHE_VAL(ec_cv_cc_visibility, [
      ec_save_CFLAGS="$CFLAGS"
      CFLAGS="$CFLAGS -fvisibility=hidden"
      AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([[
        ]], [[
extern __attribute__((__visibility__("hidden"))) int hiddenvar;
extern __attribute__((__visibility__("default"))) int exportedvar;
extern __attribute__((__visibility__("hidden"))) int hiddenfunc (void);
extern __attribute__((__visibility__("default"))) int exportedfunc (void);
        ]])
      ],
        ec_cv_cc_visibility=yes,
        ec_cv_cc_visibility=no)
      CFLAGS="$ec_save_CFLAGS"])
    AC_MSG_RESULT([$ec_cv_cc_visibility])
    if test $ec_cv_cc_visibility = yes; then
      CFLAGS_VISIBILITY="-fvisibility=hidden"
      HAVE_VISIBILITY=1
      AC_DEFINE(__EXPORT__, __attribute__((__visibility__("default"))), [Symbol is exported])
    fi
  else
    enable_visibility_hiding=no
  fi

  AC_SUBST([CFLAGS_VISIBILITY])
  AC_SUBST([HAVE_VISIBILITY])
  AC_DEFINE_UNQUOTED([HAVE_VISIBILITY], [$HAVE_VISIBILITY],
    [Define to 1 or 0, depending whether the compiler supports simple visibility declarations.])
])
