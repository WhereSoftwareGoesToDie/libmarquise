AM_CFLAGS  = -pthread -Wall $(GLIB_2_CFLAGS) # $(PROTOBUF_C_CFLAGS) -g
AM_LDFLAGS = $(GLIB_2_LIBS) -lprotobuf-c -pthread # $(PROTOBUF_C_LIBS) -- debian bug #731726

lib_LTLIBRARIES = libmarquise.la
libmarquise_la_LDFLAGS = $(AM_LDFLAGS) -version-info 1:0:0
libmarquise_la_SOURCES = marquise.c 
include_HEADERS = marquise.h

indent: *.c *.h bin/*.c tests/*.c
	indent -linux $^
	rm $(foreach input,$^,$(input)~)

test: check
