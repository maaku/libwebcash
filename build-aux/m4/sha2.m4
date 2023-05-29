AC_DEFUN([AX_CHECK_SHA2], [
    AC_ARG_WITH([included-sha2],
                [AC_HELP_STRING([--with-included-sha2],
                                [use the included libsha2 instead of system\'s])])
    AC_CHECK_LIB([sha2], [sha256_init],
                 [need_included_sha2=no],
                 [need_included_sha2=yes])

    if test "x$need_included_sha2" = "xyes"; then
        if test "x$with_included_sha2" = "xno"; then
            AC_MSG_ERROR([Could not link with system\'s libsha2.  Please make sure it is installed, or use --with-included-sha2.])
        else
            with_included_sha2=yes
        fi
    fi

    if test "x$with_included_sha2" = "xyes"; then
        SHA2_LDFLAGS='$(top_builddir)/depends/libsha2/lib/.libs/libsha2.a'
        SHA2_CPPFLAGS='-I$(top_srcdir)/depends/libsha2/include'
    else
        SHA2_LDFLAGS='-lsha2'
    fi

    # We always configure the local copy of libsha2.  This is needed to ensure
    # that it is distributed properly, irregarless of whether we use it or not
    # in this build.
    AC_CONFIG_SUBDIRS([depends/libsha2])

    AM_CONDITIONAL([WITH_INCLUDED_SHA2], [test "x$with_included_sha2" = "xyes"])
    AC_SUBST([SHA2_LDFLAGS])
    AC_SUBST([SHA2_CPPFLAGS])
])
