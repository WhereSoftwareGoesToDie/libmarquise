AM_CFLAGS  = -pthread -Wall $(GLIB_2_CFLAGS) 
AM_LDFLAGS = $(GLIB_2_LIBS) -pthread 

lib_LTLIBRARIES = libmarquise.la
libmarquise_la_LDFLAGS = $(AM_LDFLAGS) -version-info 1:0:0
libmarquise_la_SOURCES = marquise.c siphash24.c
include_HEADERS = marquise.h
dist_noinst_HEADERS = siphash24.h

indent: *.c *.h bin/*.c tests/*.c
	indent -linux $^
	rm $(foreach input,$^,$(input)~)

test: check
