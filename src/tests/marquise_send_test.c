#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "../marquise.h"

void test_send_simple() {
	setenv("MARQUISE_SPOOL_DIR", "/tmp", 1);
	mkdir("/tmp/marquisetest", 0700);
	marquise_ctx *ctx = marquise_init("marquisetest");
	if (ctx == NULL) {
		printf("marquise_init failed: %s\n", strerror(errno));
		g_test_fail();
	}
	marquise_send_simple(ctx, 5, 100, 200000);
	marquise_send_simple(ctx, 5, 101, 200001);
	marquise_send_simple(ctx, 5, 102, 200002);
	marquise_send_simple(ctx, 5, 103, 200003);
}

int main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/marquise_send/send_simple", test_send_simple);
	return g_test_run();
}
