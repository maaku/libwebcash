ACLOCAL_AMFLAGS = -I build-aux/m4

if WITH_INCLUDED_BSTRING
    MAYBE_BSTRING = depends/libsha2
endif WITH_INCLUDED_BSTRING
if WITH_INCLUDED_SHA2
    MAYBE_LIBSHA2 = depends/libsha2
endif WITH_INCLUDED_SHA2
SUBDIRS := $(MAYBE_LIBSHA2) $(MAYBE_BSTRING) lib test
check check-local: SUBDIRS := $(filter-out depends/bstring, $(filter-out depends/libsha2, $(SUBDIRS)))
