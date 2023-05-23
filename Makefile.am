ACLOCAL_AMFLAGS = -I build-aux/m4

if WITH_INCLUDED_SHA2
    MAYBE_LIBSHA2 = depends/libsha2
endif WITH_INCLUDED_SHA2
SUBDIRS := $(MAYBE_LIBSHA2) lib test
check check-local: SUBDIRS := $(filter-out depends/libsha2, $(SUBDIRS))
