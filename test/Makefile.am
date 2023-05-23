AM_CXXFLAGS = -std=c++17

check_LTLIBRARIES = libgtest.la
libgtest_la_SOURCES = $(top_srcdir)/depends/googletest/googletest/src/gtest-all.cc
libgtest_la_CPPFLAGS = -I$(top_srcdir)/depends/googletest/googletest/include -I$(top_srcdir)/depends/googletest/googletest
libgtest_la_LDFLAGS = -pthread

check_PROGRAMS = webcash
webcash_SOURCES = webcash.cc
webcash_LDADD = libgtest.la $(top_srcdir)/lib/.libs/libwebcash.a $(BSTRING_LDFLAGS) $(SHA2_LDFLAGS)
webcash_LDFLAGS = -pthread
webcash_CPPFLAGS = -I$(top_srcdir)/depends/googletest/googletest/include -I$(top_srcdir)/depends/googletest/googletest -pthread $(BSTRING_CPPFLAGS) $(SHA2_CPPFLAGS) -I$(top_srcdir)/include

TESTS = webcash
