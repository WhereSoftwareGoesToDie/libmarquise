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

void test_build_spool_path() {
	char *spool_path = build_spool_path("/tmp", "marquisetest");
	char *expected_path = "/tmp/marquisetest/";
	size_t expected_len = strlen(expected_path);
	int i;
	for (i=0; i < expected_len; i++) {
		if (expected_path[i] != spool_path[i]) {
			printf("Got path %s, expected path with prefix %s\n", spool_path, expected_path);
			g_test_fail();
		}
	}
}

int main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/valid_namespace/valid", test_valid_namespace);
	g_test_add_func("/valid_namespace/invalid", test_invalid_namespace);
	g_test_add_func("/build_spool_path/path", test_build_spool_path);
	return g_test_run();
}
