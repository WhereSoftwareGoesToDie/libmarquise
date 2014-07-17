#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "../marquise.h"

#define SIMPLE_ADDRESS   1234567890123456780
#define SIMPLE_TIMESTAMP 1405392588998566144
#define SIMPLE_VALUE     133713371337
#define EXTENDED_ADDRESS   1234567890999999999
#define EXTENDED_TIMESTAMP 1405392588999999999
#define EXTENDED_VALUE     "This is data これはデータ and Sinhala ශුද්ධ සිංහල"
#define EXTENDED_VALUE_LEN sizeof(EXTENDED_VALUE)-1


void test_send_simple() {
	setenv("MARQUISE_SPOOL_DIR", "/tmp", 1);
	mkdir("/tmp/marquisetest", 0700);
	marquise_ctx *ctx = marquise_init("marquisetest");
	if (ctx == NULL) {
		perror("marquise_init failed");
		g_test_fail();
		return;
	}

	if (marquise_send_simple(ctx, SIMPLE_ADDRESS, SIMPLE_TIMESTAMP, SIMPLE_VALUE) != 0) {
		perror("marquise_send_simple failed");
		g_test_fail();
		return;
	}
}

void test_send_extended() {
	setenv("MARQUISE_SPOOL_DIR", "/tmp", 1);
	mkdir("/tmp/marquisetest", 0700);
	marquise_ctx *ctx = marquise_init("marquisetest");
	if (ctx == NULL) {
		perror("marquise_init failed");
		g_test_fail();
	}

	if (marquise_send_extended(ctx, EXTENDED_ADDRESS, EXTENDED_TIMESTAMP, EXTENDED_VALUE, EXTENDED_VALUE_LEN) != 0) {
		perror("marquise_send_extended failed");
		g_test_fail();
		return;
	}
}

int main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/marquise_send/send_simple", test_send_simple);
	g_test_add_func("/marquise_send/send_extended", test_send_extended);
	return g_test_run();
}
