AM_CFLAGS  = -pthread -Wall `pkg-config --cflags --libs glib-2.0`
AM_LDFLAGS = -lzmq
lib_LTLIBRARIES = libanchor_stats.la
libanchor_stats_la_SOURCES = anchor_stats.c
include_HEADERS = anchor_stats.h

bin_PROGRAMS = send_anchor_stat
send_anchor_stat_SOURCES = send_anchor_stat.c
send_anchor_stat_LDADD = $(LIBOBJS) libanchor_stats.la
