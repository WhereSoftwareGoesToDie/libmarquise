AM_CFLAGS  = -pthread -Wall $(GLIB_2_CFLAGS) # $(PROTOBUF_C_CFLAGS)
AM_LDFLAGS = $(GLIB_2_LIBS) -lprotobuf-c # $(PROTOBUF_C_LIBS) -- debian bug #731726

lib_LTLIBRARIES = libmarquise.la
libmarquise_la_LDFLAGS = $(AM_LDFLAGS) -version-info 1:0:0
libmarquise_la_SOURCES = \
	protobuf/DataFrame.pb-c.c \
	protobuf/DataBurst.pb-c.c \
	marquise.c \
	frame.c \
	defer.c \
	varint.c \
	lz4/lz4.c \
	lz4/lz4hc.c
include_HEADERS = marquise.h
dist_noinst_HEADERS = defer.h \
		      macros.h \
		      structs.h \
		      frame.h \
		      varint.h \
		      protobuf/DataBurst.pb-c.h \
		      protobuf/DataFrame.pb-c.h \
		      frame.h \
		      varint.h

dist_noinst_DATA = lz4/*.c lz4/*.h

bin_PROGRAMS = send_anchor_stat
send_anchor_stat_SOURCES = send_anchor_stat.c
send_anchor_stat_LDADD = libmarquise.la

%.pb-c.c %.pb-c.h: %.proto
	    $(PROTOC_C) --proto_path=$(srcdir)/protobuf --c_out=$(builddir)/protobuf $^

MOSTLYCLEANFILES = protobuf/*.o protobuf/*.lo

TESTS=$(check_PROGRAMS)
check_PROGRAMS=\
	defer_file_test \
	marquise_consumer_new_test \
	marquise_connect_test \
	varint_test \
	full_stack_test

defer_file_test_SOURCES = tests/defer_file_test.c 
defer_file_test_LDADD = libmarquise.la

marquise_consumer_new_test_SOURCES = tests/marquise_consumer_new_test.c
marquise_consumer_new_test_LDADD = libmarquise.la

marquise_connect_test_SOURCES = tests/marquise_connect_test.c
marquise_connect_test_LDADD = libmarquise.la

varint_test_SOURCES = tests/varint_test.c
varint_test_LDADD = libmarquise.la

full_stack_test_SOURCES = tests/full_stack_test.c
full_stack_test_LDADD = libmarquise.la

test: check
