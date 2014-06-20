#include <glib.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../marquise.h"

extern uint8_t valid_namespace(char *namespace);
extern char* build_spool_path(const char *spool_prefix, char *namespace);

void test_valid_namespace() {
	int ret = valid_namespace("abcdefghijklmn12345");
	g_assert_cmpint(ret, ==, 1);
}

void test_invalid_namespace() {
	int ret = valid_namespace("a_b");
	g_assert_cmpint(ret, ==, 0);
}

int main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/valid_namespace/valid", test_valid_namespace);
	g_test_add_func("/valid_namespace/invalid", test_invalid_namespace);
	return g_test_run();
}
