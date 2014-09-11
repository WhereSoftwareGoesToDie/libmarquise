#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../marquise.h"

#define SIMPLE_ADDRESS   1234567890123456780
#define SIMPLE_TIMESTAMP 1405392588998566144
#define SIMPLE_VALUE     133713371337

void test_rotate() {
    int max_simple_per_file = MAX_SPOOL_FILE_SIZE/ 24;
    int i;
    setenv("MARQUISE_SPOOL_DIR", "/tmp", 1);
	marquise_ctx *ctx = marquise_init("marquisetest");
	if (ctx == NULL) {
		printf("marquise_init failed: %s\n", strerror(errno));
		g_test_fail();
		return;
    }
    char *initial_points_file = strdup(ctx->spool_path[SPOOL_POINTS]);
    for (i = 0; i < max_simple_per_file; i++) {
        marquise_send_simple(ctx, SIMPLE_ADDRESS, SIMPLE_TIMESTAMP + i, SIMPLE_VALUE);
    }
    int ret = strcmp(initial_points_file, ctx->spool_path[SPOOL_POINTS]);
    if (ret != 0) {
        printf("rotation of spool files failed: should not have rotated but did\n");
        g_test_fail();
        return;
    }
    marquise_send_simple(ctx, SIMPLE_ADDRESS, SIMPLE_TIMESTAMP + max_simple_per_file, SIMPLE_VALUE);
    ret = strcmp(initial_points_file, ctx->spool_path[SPOOL_POINTS]);
    if (ret == 0) {
        printf("rotation of spool files failed: should have rotated but did not\n");
        g_test_fail();
        return;
    }
}

int main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/marquise_rotate/rotate", test_rotate);
	return g_test_run();

}
