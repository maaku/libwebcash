AC_PREREQ([2.60])
AC_INIT([libwebcash],[0.1])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])

AC_CANONICAL_HOST

AH_TOP([#ifndef LIBWEBCASH__CONFIG_H])
AH_TOP([#define LIBWEBCASH__CONFIG_H 1])
AH_BOTTOM([#endif /*LIBWEBCASH__CONFIG_H*/])

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_PROG_AR
LT_INIT

dnl make the compilation flags quiet unless V=1 is used
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

PKG_PROG_PKG_CONFIG

AC_PATH_TOOL(AR, ar)
AC_PATH_TOOL(RANLIB, ranlib)
AC_PATH_TOOL(STRIP, strip)

if test "x$CFLAGS" = "x"; then
    CFLAGS="-O3 -g"
fi

AC_PROG_CC
if test x"$ac_cv_prog_cc_c89" = x"no"; then
    AC_MSG_ERROR([c89 compiler support required])
fi
AM_PROG_CC_C_O

AC_PROG_CXX

CFLAGS="$CFLAGS -W"

warn_CFLAGS="-std=c89 -pedantic -Wall -Wextra -Wcast-align -Wnested-externs -Wshadow -Wstrict-prototypes -Wno-unused-function -Wno-long-long -Wno-overlength-strings"
saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $warn_CFLAGS"
AC_MSG_CHECKING([if ${CC} supports ${warn_CFLAGS}])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[char foo;]])],
    [ AC_MSG_RESULT([yes]) ],
    [ AC_MSG_RESULT([no])
      CFLAGS="$saved_CFLAGS"
    ])

saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fvisibility=hidden"
AC_MSG_CHECKING([if ${CC} supports -fvisibility=hidden])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[char foo;]])],
    [ AC_MSG_RESULT([yes]) ],
    [ AC_MSG_RESULT([no])
      CFLAGS="$saved_CFLAGS"
    ])

AC_C_INLINE
AC_C_RESTRICT

AX_CHECK_COMPILE_FLAG([-Werror], [CFLAG_WERROR="-Werror"], [CFLAG_WERROR=""])

dnl libsha2 -- use system or included?
AX_CHECK_SHA2

AC_CONFIG_HEADERS([lib/config/libwebcash-config.h])
AC_CONFIG_FILES([Makefile lib/Makefile lib/libwebcash.pc test/Makefile])

dnl make sure nothing new is exported so that we don't break the cache
PKGCONFIG_PATH_TEMP="$PKG_CONFIG_PATH"
unset PKG_CONFIG_PATH
PKG_CONFIG_PATH="$PKGCONFIG_PATH_TEMP"

AC_OUTPUT
