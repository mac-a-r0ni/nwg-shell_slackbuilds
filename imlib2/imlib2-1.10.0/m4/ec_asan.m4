dnl Copyright (C) 2020 Kim Woelders
dnl This code is public domain and can be freely used or copied.

dnl Macro to set compiler flags in CFLAGS_ASAN

dnl Usage: EC_C_ASAN()

AC_DEFUN([EC_C_ASAN], [
  AC_ARG_ENABLE(gcc-asan,
    [AS_HELP_STRING([--enable-gcc-asan],
                    [compile with ASAN support @<:@default=no@:>@])],,
    enable_gcc_asan=no)

  if test "x$GCC" = "xyes"; then
    if test "x$enable_gcc_asan" = "xyes"; then
      CFLAGS_ASAN="-fsanitize=address -fno-omit-frame-pointer"
    fi
  fi
  AC_SUBST(CFLAGS_ASAN)
])
