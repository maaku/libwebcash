AM_CXXFLAGS = -std=c++17

check_LTLIBRARIES = libgtest.la
libgtest_la_SOURCES = $(top_srcdir)/depends/googletest/googletest/src/gtest-all.cc
libgtest_la_CPPFLAGS = -I$(top_srcdir)/depends/googletest/googletest/include -I$(top_srcdir)/depends/googletest/googletest
libgtest_la_LDFLAGS = -pthread

check_PROGRAMS = webcash
webcash_SOURCES = webcash.cc
webcash_LDADD = libgtest.la $(top_srcdir)/lib/.libs/libwebcash.a $(top_srcdir)/depends/bstring/bstring/.libs/libbstring.a $(top_srcdir)/depends/libsha2/lib/.libs/libsha2.a
webcash_LDFLAGS = -pthread
webcash_CPPFLAGS = -I$(top_srcdir)/depends/googletest/googletest/include -I$(top_srcdir)/depends/googletest/googletest -pthread -I$(top_srcdir)/depends/bstring/bstring -I$(top_srcdir)/depends/libsha2/include -I$(top_srcdir)/include

TESTS = webcash
