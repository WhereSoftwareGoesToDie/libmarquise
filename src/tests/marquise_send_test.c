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
#define EXTENDED_VALUE     "hello world"
#define EXTENDED_VALUE_LEN 11  // yes this is evil/dangerous, this is measured in bytes, which will be non-obvious if you have a non-ASCII UTF-8 test string


void test_send_simple() {
	int ret;

	setenv("MARQUISE_SPOOL_DIR", "/tmp", 1);
	mkdir("/tmp/marquisetest", 0700);
	marquise_ctx *ctx = marquise_init("marquisetest");
	if (ctx == NULL) {
		printf("marquise_init failed: %s\n", strerror(errno));
		g_test_fail();
		return;
	}

	ret = marquise_send_simple(ctx, SIMPLE_ADDRESS, SIMPLE_TIMESTAMP, SIMPLE_VALUE);
	if (ret != 0) {
		printf("marquise_send_simple failed: %s\n", strerror(errno));
		g_test_fail();
		return;
	}
}

void test_send_extended() {
	int ret;

	setenv("MARQUISE_SPOOL_DIR", "/tmp", 1);
	mkdir("/tmp/marquisetest", 0700);
	marquise_ctx *ctx = marquise_init("marquisetest");
	if (ctx == NULL) {
		printf("marquise_init failed: %s\n", strerror(errno));
		g_test_fail();
	}

	ret = marquise_send_extended(ctx, EXTENDED_ADDRESS, EXTENDED_TIMESTAMP, EXTENDED_VALUE, EXTENDED_VALUE_LEN);
	if (ret != 0) {
		printf("marquise_send_extended failed: %s\n", strerror(errno));
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
