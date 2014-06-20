#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../marquise.h"

void test_init() {
	setenv("MARQUISE_SPOOL_DIR", "/tmp", 1);
	mkdir("/tmp/marquisetest", 0700);
	marquise_ctx *ctx = marquise_init("marquisetest");
	g_assert_nonnull(ctx);
}

int main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/marquise_init/init", test_init);
	return g_test_run();
}
