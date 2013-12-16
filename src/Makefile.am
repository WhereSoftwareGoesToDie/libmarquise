AM_CFLAGS  = -pthread -Wall $(DEPS_CFLAGS)
AM_LDFLAGS = $(DEPS_LIBS)

lib_LTLIBRARIES = libanchor_stats.la
libanchor_stats_la_SOURCES = anchor_stats.c 
	protobuf/DataFrame.pb-c.c
	protobuf/DataBurst.pb-c.c
libanchor_stats_la_LIBS = zmq protobuf $(DEPS_LIBS)

include_HEADERS = anchor_stats.h

bin_PROGRAMS = send_anchor_stat
send_anchor_stat_SOURCES = send_anchor_stat.c
send_anchor_stat_LDADD = $(LIBOBJS) libanchor_stats.la

dist_noinst_DATA= protobuf/DataBurst.proto protobuf/DataFrame.proto
%.pb-c.c %.pb-c.h: %.proto
	    $(PROTOC_C) --proto_path=$(srcdir) --c_out=$(builddir) $^

MOSTLYCLEANFILES = protobuf/*.c protobuf/*.h protobuf/*.o protobuf/*.lo
