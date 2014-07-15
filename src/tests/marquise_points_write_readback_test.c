#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "../marquise.h"

/* Read 64 bits from little-endian byte array p, make a 64-bit value. */
#define LE8TOU64(v, p) v = \
	(((uint64_t)p[0])      ) + \
	(((uint64_t)p[1]) <<  8) + \
	(((uint64_t)p[2]) << 16) + \
	(((uint64_t)p[3]) << 24) + \
	(((uint64_t)p[4]) << 32) + \
	(((uint64_t)p[5]) << 40) + \
	(((uint64_t)p[6]) << 48) + \
	(((uint64_t)p[7]) << 56)


void test_points_write_readback() {
	/* Be careful with the addresses we write, the LSB will be cleared for
	   simple points and set for extended points. */

	int ret;

	// Init
	setenv("MARQUISE_SPOOL_DIR", "/tmp", 1);
	marquise_ctx *ctx = marquise_init("marquisetest");
	if (ctx == NULL) {
		printf("marquise_init failed: %s\n", strerror(errno));
		g_test_fail();
		return;
	}

	// Write simple (address/timestamp/value)
	ret = marquise_send_simple(ctx, 1234567890123456780, 1405392588998566144, 133713371337);
	if (ret != 0) {
		printf("marquise_send_simple failed: %s\n", strerror(errno));
		g_test_fail();
		return;
	}

	// Write extended (address/timestamp/value/length)
	ret = marquise_send_extended(ctx, 1234567890999999999, 1405392588999999999, "hello world", 11);
	if (ret != 0) {
		printf("marquise_send_extended failed: %s\n", strerror(errno));
		g_test_fail();
		return;
	}

	// Finalise and save spool path
	char* written_spool_path = strdup(ctx->spool_path_points);
	ret = marquise_shutdown(ctx);
	if (ret != 0) {
		printf("marquise_shutdown failed: %s\n", strerror(errno));
		g_test_fail();
		return;
	}

	// Reopen the spool file and prepare
	FILE *spool = fopen(written_spool_path, "r");
	if (spool == NULL) {
		free(written_spool_path);
		printf("failed to reopen spool file for reading: %s\n", strerror(errno));
		g_test_fail();
		return;
	}
	// XXX: can we free(written_spool_path) now? Probably, once the file is open.

	uint64_t address;
	uint64_t timestamp;
	uint64_t value;
	unsigned char simple_buf[24];

	// Read back a simple data point
	ret = fread(simple_buf, 1, 24, spool);
	if (ret != 24) {
		if (feof(spool) != 0) {
			printf("Unexpected short read of %d bytes from spool while reading back simple data point, end of file\n", ret);
		}
		if (ferror(spool) != 0) {
			printf("Unexpected short read of %d bytes from spool while reading back simple data point, there was an error\n", ret);
		}
		printf("Can't continue after short read\n");
		g_test_fail();
		return;
	}
	LE8TOU64(address,    simple_buf);
	LE8TOU64(timestamp, (simple_buf+8));
	LE8TOU64(value,     (simple_buf+16));

	if (address != 1234567890123456780) {
		free(written_spool_path);
		fclose(spool);
		printf("Returned value of 'address' %lu doesn't match expected values of %lu\n", address, 1234567890123456780);
		g_test_fail();
		return;
	}

	if (timestamp != 1405392588998566144) {
		free(written_spool_path);
		fclose(spool);
		printf("Returned value of 'timestamp' %lu doesn't match expected values of %lu\n", timestamp, 1405392588998566144);
		g_test_fail();
		return;
	}

	if (value != 133713371337) {
		free(written_spool_path);
		fclose(spool);
		printf("Returned value of 'value' %lu doesn't match expected values of %lu\n", value, 133713371337);
		g_test_fail();
		return;
	}

	// Read back extended
	// XXX: To be written
	
	// Cleanup and finish
	free(written_spool_path);
	ret = fclose(spool);
	if (ret != 0) {
		printf("failed to close spool file for reread as final action, meh: %s\n", strerror(errno));
		g_test_fail();
	}
}

int main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/marquise_points_write_readback/points_write_readback", test_points_write_readback);
	return g_test_run();
}
