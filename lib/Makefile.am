lib_LTLIBRARIES = libwebcash.la
webcashincludedir = $(includedir)
webcashinclude_HEADERS = $(top_srcdir)/include/webcash.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libwebcash.pc

libwebcash_la_CPPFLAGS = -I$(top_srcdir)/include $(BSTRING_CPPFLAGS) $(SHA2_CPPFLAGS)
libwebcash_la_SOURCES = webcash.c
