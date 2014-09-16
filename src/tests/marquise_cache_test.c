#include <glib-2.0/glib.h>
#include <stdlib.h>
#include <string.h>

#include "../marquise.h"

#define TEST_ADDRESS                1234567890123456789
#define TEST_SOURCE_DICT_SERIALISED "foo:one,bar:two,baz:three" // len=25
#define TEST_SOURCE_DICT_LENGTH     sizeof(TEST_SOURCE_DICT_SERIALISED)-1

void test_cache() {
	int ret;
	char* fields[3] = { "foo", "bar", "baz" };
	char* values[3] = { "one", "two", "three" };

	// Init
	setenv("MARQUISE_SPOOL_DIR", "/tmp", 1);
	marquise_ctx *ctx = marquise_init("marquisetest");
	if (ctx == NULL) {
		perror("marquise_init failed");
		g_test_fail();
		return;
	}


	// Prepare, dispatch, and cleanup the source dict
	marquise_source* test_src = marquise_new_source(fields, values, 3);
	if (test_src == NULL) {
		perror("marquise_new_source failed");
		g_test_fail();
		return;
	}

	ret = marquise_update_source(ctx, TEST_ADDRESS, test_src);
	if (ret != 0) {
		perror("marquise_update_source failed");
		g_test_fail();
		return;
	}
	size_t init_bytes_written = ctx->bytes_written[SPOOL_CONTENTS];
	char *init_path = strdup(ctx->spool_path[SPOOL_CONTENTS]);
	ret = marquise_update_source(ctx, TEST_ADDRESS, test_src);
	if (ret != 0) {
		perror("marquise_update_source failed");
		g_test_fail();
		return;
	}
	if (ctx->bytes_written[SPOOL_CONTENTS] != init_bytes_written || strcmp(ctx->spool_path[SPOOL_CONTENTS], init_path) != 0) {
		printf("marquise_update_source queued a redundant source dict\n");
		g_test_fail();
		return;	
	}
	marquise_free_source(test_src);
}

int main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/marquise_cache/cache", test_cache);
	return g_test_run();
}
