AM_CFLAGS  = -pthread -Wall $(GLIB_2_CFLAGS) # $(PROTOBUF_C_CFLAGS)
AM_LDFLAGS = $(GLIB_2_LIBS) -lprotobuf-c # $(PROTOBUF_C_LIBS) -- debian bug #731726

lib_LTLIBRARIES = libanchor_stats.la
libanchor_stats_la_SOURCES = \
	protobuf/DataFrame.pb-c.c \
	protobuf/DataBurst.pb-c.c \
	anchor_stats.c \
	frame.c \
	defer.c \
	lz4/lz4.c
include_HEADERS = anchor_stats.h frame.h

bin_PROGRAMS = send_anchor_stat
send_anchor_stat_SOURCES = send_anchor_stat.c
send_anchor_stat_LDADD = libanchor_stats.la

dist_noinst_DATA = protobuf/DataBurst.proto protobuf/DataFrame.proto
%.pb-c.c %.pb-c.h: %.proto
	    $(PROTOC_C) --proto_path=$(srcdir)/protobuf --c_out=$(builddir)/protobuf $^

MOSTLYCLEANFILES = protobuf/*.c protobuf/*.h protobuf/*.o protobuf/*.lo

TESTS=$(check_PROGRAMS)
check_PROGRAMS=defer_file_test
defer_file_test_SOURCES= tests/defer_file_test.c
defer_file_test_LDADD = libanchor_stats.la

