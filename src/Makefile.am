AM_CFLAGS  = -Wall $(GLIB_2_CFLAGS) 
AM_LDFLAGS = $(GLIB_2_LIBS) 

lib_LTLIBRARIES = libmarquise.la
libmarquise_la_LDFLAGS = $(AM_LDFLAGS) -version-info 2:0:0
libmarquise_la_SOURCES = marquise.c siphash24.c
include_HEADERS = marquise.h
dist_noinst_HEADERS = siphash24.h

TESTS=$(check_PROGRAMS)
check_PROGRAMS=\
	marquise_init_test \
	marquise_hash_test \
	marquise_util_test \
	marquise_send_test \
	marquise_shutdown_test \
	marquise_points_write_readback_test \
	marquise_contents_write_readback_test \
	marquise_rotate_test \
    marquise_cache_test

marquise_hash_test_SOURCES = tests/marquise_hash_test.c
marquise_hash_test_LDADD = libmarquise.la

marquise_init_test_SOURCES = tests/marquise_init_test.c
marquise_init_test_LDADD = libmarquise.la

marquise_util_test_SOURCES = tests/marquise_util_test.c
marquise_util_test_LDADD = libmarquise.la

marquise_send_test_SOURCES = tests/marquise_send_test.c
marquise_send_test_LDADD = libmarquise.la

marquise_shutdown_test_SOURCES = tests/marquise_shutdown_test.c
marquise_shutdown_test_LDADD = libmarquise.la

marquise_points_write_readback_test_SOURCES = tests/marquise_points_write_readback_test.c
marquise_points_write_readback_test_LDADD = libmarquise.la

marquise_contents_write_readback_test_SOURCES = tests/marquise_contents_write_readback_test.c
marquise_contents_write_readback_test_LDADD = libmarquise.la

marquise_rotate_test_SOURCES = tests/marquise_rotate_test.c
marquise_rotate_test_LDADD = libmarquise.la

marquise_cache_test_SOURCES = tests/marquise_cache_test.c
marquise_cache_test_LDADD = libmarquise.la

indent: *.c *.h bin/*.c tests/*.c
	indent -linux $^
	rm $(foreach input,$^,$(input)~)

test: check
