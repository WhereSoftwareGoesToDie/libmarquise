#include <glib.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include "../envvar.h"

void test_envvar() {
        char buf[100];
        int test_val = rand();
        srand(time(NULL));
        sprintf(buf, "%d", test_val);
        int ret = setenv("LIBMARQUISE_FOO", buf, 1);
        g_assert_cmpint(ret, ==, 0);
        int val;
        ret = get_envvar_int("LIBMARQUISE_FOO", &val);
        g_assert_cmpint(val, ==, test_val);
        g_assert_cmpint(ret, ==, 1);
        ret = setenv("LIBMARQUISE_BAR", "bar", 1);
        g_assert_cmpint(ret, ==, 0);
        ret = get_envvar_int("LIBMARQUISE_BAR", &val);
        g_assert_cmpint(ret, ==, -1);
        ret = get_envvar_int("LIBMARQUISE_DOES_NOT_EXIST", &val);
        g_assert_cmpint(ret, ==, 0);
}

int main( int argc, char **argv ){
        g_test_init( &argc, &argv, NULL);
        g_test_add_func( "/get_envvar_int/envvar"
                       , test_envvar );
        return g_test_run();
}
