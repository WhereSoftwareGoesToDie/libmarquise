AM_CFLAGS  = -pthread -Wall `pkg-config --cflags --libs glib-2.0`
AM_LDFLAGS = -lzmq -lprotobuf-c
lib_LTLIBRARIES = libanchor_stats.la
libanchor_stats_la_SOURCES = anchor_stats.c DataFrame.pb-c.h DataFrame.pb-c.c DataBurst.pb-c.h DataBurst.pb-c.c
BUILT_SOURCES = DataFrame.pb-c.h DataFrame.pb-c.c DataBurst.pb-c.h DataBurst.pb-c.c
libanchor_stats_la_SOURCES += BUILT_SOURCES
include_HEADERS = anchor_stats.h

bin_PROGRAMS = send_anchor_stat
send_anchor_stat_SOURCES = send_anchor_stat.c
send_anchor_stat_LDADD = $(LIBOBJS) libanchor_stats.la

DataFrame.pb-c.h: protobuf
DataFrame.pb-c.c: protobuf
DataBurst.pb-c.h: protobuf
DataBurst.pb-c.c: protobuf

protobuf:
	cd protobuf ; protoc-c --c_out=.. *.proto

.PHONY : protobuf
