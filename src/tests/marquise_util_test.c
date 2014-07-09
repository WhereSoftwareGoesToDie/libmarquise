#include <glib.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "../marquise.h"

extern uint8_t valid_namespace(char *namespace);
extern uint8_t valid_source_tag(char *tag);
extern char* build_spool_path(const char *spool_prefix, char *namespace, const char* spool_type);

void test_valid_namespace() {
	int ret = valid_namespace("abcdefghijklmn12345");
	g_assert_cmpint(ret, ==, 1);
}

void test_valid_source_tag() {
	int ret = valid_source_tag("-/Az1!~()=[];'\\");
	g_assert_cmpint(ret, ==, 1);
}

void test_invalid_source_tag() {
	int ret = valid_source_tag("-/Az1,!~()=[];'\\");
	g_assert_cmpint(ret, ==, 0);
	ret = valid_source_tag("-/Az1:!~()=[];'\\");
	g_assert_cmpint(ret, ==, 0);
}

void test_invalid_namespace() {
	int ret = valid_namespace("a_b");
	g_assert_cmpint(ret, ==, 0);
}

void test_build_spool_path() {
	const char *prefix = "/tmp";
	char *namespace = "marquisetest";
	char *expected_points_path   = "/tmp/marquisetest/points/new";
	char *expected_contents_path = "/tmp/marquisetest/contents/new";

	char *spool_path_points = build_spool_path(prefix, namespace, "points");
	if (spool_path_points == NULL) {
		printf("build_spool_path returned NULL when building for 'points'\n");
		printf("errno is %s\n", strerror(errno));
		free(spool_path_points);
		g_test_fail();
		return;
	}
	/* Use the length of the expected path as our safety boundary, it's */
	/* guaranteed to be safely null-terminated */
	if (strncmp(spool_path_points, expected_points_path, strlen(expected_points_path)) != 0) {
		printf("Got 'points' path %s, expected path with prefix %s\n", spool_path_points, expected_points_path);
		free(spool_path_points);
		g_test_fail();
		return;
	}
	free(spool_path_points);


	/* Test for "contents" as well. */
	char *spool_path_contents = build_spool_path(prefix, namespace, "contents");
	if (spool_path_contents == NULL) {
		printf("build_spool_path returned NULL when building for 'contents'\n");
		printf("errno is %s\n", strerror(errno));
		free(spool_path_contents);
		g_test_fail();
		return;
	}
	if (strncmp(spool_path_contents, expected_contents_path, strlen(expected_contents_path)) != 0) {
		printf("Got 'contents' path %s, expected path with prefix %s\n", spool_path_contents, expected_contents_path);
		free(spool_path_contents);
		g_test_fail();
		return;
	}
	free(spool_path_contents);
}

int main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/valid_namespace/valid", test_valid_namespace);
	g_test_add_func("/valid_namespace/invalid", test_invalid_namespace);
	g_test_add_func("/valid_source_tag/valid", test_valid_source_tag);
	g_test_add_func("/valid_source_tag/invalid", test_invalid_source_tag);
	g_test_add_func("/build_spool_path/path", test_build_spool_path);
	return g_test_run();
}
