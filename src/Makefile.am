AM_CFLAGS  = -Wall $(GLIB_2_CFLAGS) 
AM_LDFLAGS = $(GLIB_2_LIBS) 

lib_LTLIBRARIES = libmarquise.la
libmarquise_la_LDFLAGS = $(AM_LDFLAGS) -version-info 2:0:0
libmarquise_la_SOURCES = marquise.c siphash24.c
include_HEADERS = marquise.h
dist_noinst_HEADERS = siphash24.h

TESTS=$(check_PROGRAMS)
check_PROGRAMS=marquise_test

marquise_test_SOURCES = tests/marquise_test.c
marquise_test_LDADD = libmarquise.la

indent: *.c *.h bin/*.c tests/*.c
	indent -linux $^
	rm $(foreach input,$^,$(input)~)

test: check
