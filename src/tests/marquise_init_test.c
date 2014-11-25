#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "../marquise.h"

void test_init() {
	setenv("MARQUISE_SPOOL_DIR", "/tmp", 1);
	mkdir("/tmp/marquisetest", 0700);
	marquise_ctx *ctx = marquise_init("marquisetest");
	if (ctx == NULL) {
		printf("marquise_init failed: %s\n", strerror(errno));
		g_test_fail();
	}
	marquise_shutdown(ctx);
}

void test_init_no_dir() {
	setenv("MARQUISE_SPOOL_DIR", "/tmp", 1);
	marquise_ctx *ctx = marquise_init("marquisetest2");
	if (ctx == NULL) {
		printf("marquise_init failed: %s\n", strerror(errno));
		g_test_fail();
	}
	marquise_shutdown(ctx);
}

void test_lock_dir() {
	setenv("MARQUISE_LOCK_DIR", "/tmp", 1);
	marquise_ctx *ctx = marquise_init("marquisetest3");
	if (ctx == NULL) {
		printf("marquise_init failed: %s\n", strerror(errno));
		g_test_fail();
	}
	marquise_shutdown(ctx);
}

void test_lock_dir_twice() {
	setenv("MARQUISE_LOCK_DIR", "/tmp", 1);

	//Create an initial marquise instance, but don't clean it up; let it run
	marquise_ctx *ctx = marquise_init("marquisetest4");
	if (ctx == NULL) {
		printf("marquise_init failed: %s\n", strerror(errno));
		g_test_fail();
	}

	//Repeating intialization of the same namespace should fail
	marquise_ctx *ctx2 = marquise_init("marquisetest4");
	if (ctx2 != NULL) { /* We expect this to fail. Let us know if it doesn't. */
		printf("marquise_init failed: %s\n", strerror(errno));
	}

	//A new namespace will work
	marquise_ctx *ctx3 = marquise_init("marquisetest5");
	if (ctx3 == NULL) {
		printf("marquise_init failed: %s\n", strerror(errno));
		g_test_fail();
	}

	// clean up all the things
	marquise_shutdown(ctx);
	marquise_shutdown(ctx3);
}

int main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/marquise_init/init", test_init);
	g_test_add_func("/marquise_init/init_no_dir", test_init_no_dir);
	g_test_add_func("/marquise_init/lock_dir", test_lock_dir);
	g_test_add_func("/marquise_init/lock_dir_twice", test_lock_dir_twice);
	return g_test_run();
}
