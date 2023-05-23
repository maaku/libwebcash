AC_DEFUN([AX_CHECK_BSTRING], [
    AC_ARG_WITH([included-bstring],
                [AC_HELP_STRING([--with-included-bstring],
                                [use the included bstring library instead of system\'s])])
    AC_CHECK_LIB([bstring], [cstr2bstr],
                 [need_included_bstring=no],
                 [need_included_bstring=yes])

    if test "x$need_included_bstring" = "xyes"; then
        if test "x$with_included_bstring" = "xno"; then
            AC_MSG_ERROR([Could not link with system\'s bstring library.  Please make sure it is installed, or use --with-included-bstring.])
        else
            with_included_bstring=yes
        fi
    fi

    if test "x$with_included_bstring" = "xyes"; then
        BSTRING_LDFLAGS='$(top_builddir)/depends/bstring/bstring/libbstring.la'
        BSTRING_CPPFLAGS='-I$(top_srcdir)/depends/bstring/bstring'
    else
        BSTRING_LDFLAGS='-lbstring'
    fi

    # We always configure the local copy of bstring.  This is needed to ensure
    # that it is distributed properly, irregarless of whether we use it or not
    # in this build.
    AC_CONFIG_SUBDIRS([depends/bstring])

    AM_CONDITIONAL([WITH_INCLUDED_BSTRING], [test "x$with_included_bstring" = "xyes"])
    AC_SUBST([BSTRING_LDFLAGS])
    AC_SUBST([BSTRING_CPPFLAGS])
])
