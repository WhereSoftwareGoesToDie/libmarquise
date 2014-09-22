#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "../marquise.h"

void test_bad_namespace() {
	setenv("MARQUISE_SPOOL_DIR", "/tmp", 1);
	marquise_ctx *ctx = marquise_init("@this is_a bad-namespace!^");
	if (ctx != NULL) {
		printf("marquise_init succeeded against all odds, that should've been a bad namespace\n");
		marquise_shutdown(ctx);
		g_test_fail();
		return;
	}
}

int main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/marquise_namespace/bad_namespace", test_bad_namespace);
	return g_test_run();
}
