#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../marquise.h"

#define TEST_ADDRESS                1234567890123456789
#define TEST_SOURCE_DICT_SERIALISED "foo:one,bar:two,baz:three" // len=25
#define TEST_SOURCE_DICT_LENGTH     sizeof(TEST_SOURCE_DICT_SERIALISED)-1

void test_cache() {
	int ret;
	char* fields[3] = { "foo", "bar", "baz" };
	char* values[3] = { "one", "two", "three" };

	char* fields2[2] = { "four", "five" };
	char* values2[2] = { "six", "seven" };

	/* Initialise context */
	setenv("MARQUISE_SPOOL_DIR", "/tmp", 1);
	setenv("MARQUISE_LOCK_DIR", "/tmp", 1);
	marquise_ctx *ctx = marquise_init("marquisetest");
	if (ctx == NULL) {
		perror("marquise_init failed");
		g_test_fail();
		return;
	}

	/* Prepare sourcedict */
	marquise_source* test_src = marquise_new_source(fields, values, 3);
	if (test_src == NULL) {
		perror("marquise_new_source failed");
		g_test_fail();
		return;
	}

	/* Dispatch sourcedict */
	ret = marquise_update_source(ctx, TEST_ADDRESS, test_src);
	if (ret != 0) {
		perror("marquise_update_source failed (1st attempt)");
		g_test_fail();
		return;
	}

	/* Collect state after 1st dispatch */
	size_t init_bytes_written = ctx->bytes_written_contents;
	char*  init_path = strdup(ctx->spool_path_contents);
	if (init_path == NULL) {
		perror("Failed to strdup() after first sourcedict dispatch");
		g_test_fail();
		return;
	}

	/* Dispatch again */
	ret = marquise_update_source(ctx, TEST_ADDRESS, test_src);
	if (ret != 0) {
		perror("marquise_update_source failed (2nd attempt)");
		g_test_fail();
		return;
	}

	/* Verify new state, we should NOT have written any new spool data
	 * thanks to the presence of the sourcedict cache.
	 */
	if (ctx->bytes_written_contents != init_bytes_written || strcmp(ctx->spool_path_contents, init_path) != 0) {
		printf("marquise_update_source queued a redundant source dict\n");
		g_test_fail();
		return;
	}

	marquise_source* test_src2 = marquise_new_source(fields2, values2, 2);
	if (test_src == NULL) {
		perror("marquise_new_source failed");
		g_test_fail();
		return;
	}
	init_bytes_written = ctx->bytes_written_contents;
	ret = marquise_update_source(ctx, TEST_ADDRESS, test_src2);
	if (ret != 0) {
		perror("marquise_update_source failed (3rd attempt)");
		g_test_fail();
		return;
	}

	/* Ensure that a sourcedict not in the cache is queued for
         * update. */
	if (ctx->bytes_written_contents != init_bytes_written + 35) {
		printf("marquise_update_source failed to queue a non-redundant source dict\n");
		g_test_fail();
		return;
	}

	/* Cleanup */
	free(init_path);
	marquise_free_source(test_src);
	ret = marquise_shutdown(ctx);
	if (ret != 0) {
		printf("marquise_shutdown failed during final cleanup: %s\n", strerror(errno));
		g_test_fail();
		return;
	}
}

int main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/marquise_cache/cache", test_cache);
	return g_test_run();
}
