#include <glib-2.0/glib.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "../marquise.h"

void test_shutdown() {
	setenv("MARQUISE_SPOOL_DIR", "/tmp", 1);
	marquise_ctx *ctx = marquise_init("marquisetest");
	if (ctx == NULL) {
		printf("marquise_init failed: %s\n", strerror(errno));
		g_test_fail();
		return;
	}
	int ret = marquise_shutdown(ctx);
	if (ret != 0) {
		printf("marquise_shutdown failed: %s\n", strerror(errno));
		g_test_fail();	
		return;
	}
}

int main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/marquise_shutdown/shutdown", test_shutdown);
	return g_test_run();
}
