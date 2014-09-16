#include <glib-2.0/glib.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "../marquise.h"

extern uint8_t valid_namespace(char *namespace);
extern uint8_t valid_source_tag(char *tag);
extern char* build_spool_path(const char *spool_prefix, char *namespace, const char* spool_type);
extern char* serialise_marquise_source(marquise_source *source);

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
		perror("build_spool_path returned NULL when building for 'points'");
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
		perror("build_spool_path returned NULL when building for 'contents'");
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
void test_serialise_marquise_source() {
	char* fields[3] = { "foo", "bar", "baz" };
	char* values[3] = { "one", "two", "three" };
	const char* expected_serialisation = "foo:one,bar:two,baz:three";

	marquise_source* test_src = marquise_new_source(fields, values, 3);
	char* serialised_dict = serialise_marquise_source(test_src);
	if (serialised_dict == NULL) {
		free(serialised_dict);
		g_test_fail();
		return;
	}

	if (strncmp(expected_serialisation, serialised_dict, strlen(expected_serialisation)) != 0) {
		free(serialised_dict);
		g_test_fail();
		return;
	}

	free(serialised_dict);
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
