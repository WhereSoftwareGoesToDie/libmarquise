AM_CFLAGS  = -pthread -Wall $(GLIB_2_CFLAGS) # $(PROTOBUF_C_CFLAGS) -g
AM_LDFLAGS = $(GLIB_2_LIBS) -lprotobuf-c -pthread # $(PROTOBUF_C_LIBS) -- debian bug #731726

lib_LTLIBRARIES = libmarquise.la
libmarquise_la_LDFLAGS = $(AM_LDFLAGS) -version-info 1:0:0
libmarquise_la_SOURCES = \
	protobuf/DataFrame.pb-c.c \
	protobuf/DataBurst.pb-c.c \
	envvar.c \
	marquise.c \
	frame.c \
	defer.c \
	telemetry.c \
	varint.c \
	lz4/lz4.c \
	lz4/lz4hc.c 
include_HEADERS = marquise.h
dist_noinst_HEADERS = defer.h \
		      macros.h \
		      structs.h \
		      frame.h \
		      varint.h \
		      telemetry.h \
		      protobuf/DataBurst.pb-c.h \
		      protobuf/DataFrame.pb-c.h \
		      frame.h \
		      varint.h \
	              envvar.h

dist_noinst_DATA = lz4/*.c lz4/*.h

bin_PROGRAMS = marquise marquised marquise_process_deferrals

marquise_SOURCES = bin/marquise.c
marquise_LDADD = libmarquise.la

marquised_SOURCES = marquised.c
marquised_LDADD = libmarquise.la

marquise_process_deferrals_SOURCES = marquise_process_deferrals.c
marquise_process_deferrals_LDADD = libmarquise.la

%.pb-c.c %.pb-c.h: %.proto
	    $(PROTOC_C) --proto_path=$(srcdir)/protobuf --c_out=$(builddir)/protobuf $^

MOSTLYCLEANFILES = protobuf/*.o protobuf/*.lo protobuf/*.pb-c.*

TESTS=$(check_PROGRAMS)
check_PROGRAMS=\
	defer_file_test \
	marquise_consumer_new_test \
	marquise_connect_test \
	varint_test \
	full_stack_test \
	envvar_test

defer_file_test_SOURCES = tests/defer_file_test.c 
defer_file_test_LDADD = libmarquise.la

marquise_consumer_new_test_SOURCES = tests/marquise_consumer_new_test.c
marquise_consumer_new_test_LDADD = libmarquise.la

marquise_connect_test_SOURCES = tests/marquise_connect_test.c
marquise_connect_test_LDADD = libmarquise.la

varint_test_SOURCES = tests/varint_test.c
varint_test_LDADD = libmarquise.la

envvar_test_SOURCES = tests/envvar_test.c
envvar_test_LDADD = libmarquise.la

full_stack_test_SOURCES = tests/full_stack_test.c
full_stack_test_LDADD = libmarquise.la

indent: *.c *.h bin/*.c tests/*.c
	indent -linux $^
	rm $(foreach input,$^,$(input)~)

test: check
