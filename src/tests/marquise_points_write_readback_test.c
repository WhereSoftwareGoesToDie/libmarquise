#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "../marquise.h"

/* Be careful with the addresses we write, the LSB will be cleared for
   simple points and set for extended points. */
#define SIMPLE_ADDRESS   1234567890123456780
#define SIMPLE_TIMESTAMP 1405392588998566144
#define SIMPLE_VALUE     133713371337
#define EXTENDED_ADDRESS   1234567890999999999
#define EXTENDED_TIMESTAMP 1405392588999999999
#define EXTENDED_VALUE     "This is data これはデータ and Sinhala ශුද්ධ සිංහල"
#define EXTENDED_VALUE_LEN sizeof(EXTENDED_VALUE)-1

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
	ret = marquise_send_simple(ctx, SIMPLE_ADDRESS, SIMPLE_TIMESTAMP, SIMPLE_VALUE);
	if (ret != 0) {
		printf("marquise_send_simple failed: %s\n", strerror(errno));
		g_test_fail();
		return;
	}

	// Write extended (address/timestamp/value/length)
	ret = marquise_send_extended(ctx, EXTENDED_ADDRESS, EXTENDED_TIMESTAMP, EXTENDED_VALUE, EXTENDED_VALUE_LEN);
	if (ret != 0) {
		printf("marquise_send_extended failed: %s\n", strerror(errno));
		g_test_fail();
		return;
	}

	// Finalise and save spool path
	char* written_spool_path = strdup(ctx->spool_path[SPOOL_POINTS]);
	if (written_spool_path == NULL) {
		printf("failed to strdup ctx->spool_path_points: %s\n", strerror(errno));
		g_test_fail();
		return;
	}
	ret = marquise_shutdown(ctx);
	if (ret != 0) {
		printf("marquise_shutdown failed: %s\n", strerror(errno));
		free(written_spool_path);
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
	free(written_spool_path);

	uint64_t address;
	uint64_t timestamp;
	uint64_t value;
	char*    extended_value;
	uint64_t extended_value_len;
	unsigned char simple_buf[24];
	unsigned char extended_header_buf[24];


	// Read back a simple data point
	ret = fread(simple_buf, 1, 24, spool);
	if (ret != 24) {
		if (feof(spool) != 0) {
			printf("Unexpected short read of %d bytes from spool while reading back simple data point, end of file\n", ret);
		}
		if (ferror(spool) != 0) {
			printf("Unexpected short read of %d bytes from spool while reading back simple data point, there was an error\n", ret);
		}
		fclose(spool);
		printf("Can't continue after short read\n");
		g_test_fail();
		return;
	}
	LE8TOU64(address,    simple_buf);
	LE8TOU64(timestamp, (simple_buf+8));
	LE8TOU64(value,     (simple_buf+16));

	if (address != SIMPLE_ADDRESS) {
		fclose(spool);
		printf("Returned value of simple 'address' %lu doesn't match expected value of %lu\n", address, SIMPLE_ADDRESS);
		g_test_fail();
		return;
	}

	if (timestamp != SIMPLE_TIMESTAMP) {
		fclose(spool);
		printf("Returned value of simple 'timestamp' %lu doesn't match expected value of %lu\n", timestamp, SIMPLE_TIMESTAMP);
		g_test_fail();
		return;
	}

	if (value != SIMPLE_VALUE) {
		fclose(spool);
		printf("Returned value of simple 'value' %lu doesn't match expected value of %lu\n", value, SIMPLE_VALUE);
		g_test_fail();
		return;
	}


	// Read back extended
	ret = fread(extended_header_buf, 1, 24, spool);
	if (ret != 24) {
		if (feof(spool) != 0) {
			printf("Unexpected short read of %d bytes from spool while reading back extended data point, end of file\n", ret);
		}
		if (ferror(spool) != 0) {
			printf("Unexpected short read of %d bytes from spool while reading back extended data point, there was an error\n", ret);
		}
		fclose(spool);
		printf("Can't continue after short read\n");
		g_test_fail();
		return;
	}
	LE8TOU64(address,             extended_header_buf);
	LE8TOU64(timestamp,          (extended_header_buf+8));
	LE8TOU64(extended_value_len, (extended_header_buf+16));

	if (address != EXTENDED_ADDRESS) {
		fclose(spool);
		printf("Returned value of extended 'address' %lu doesn't match expected value of %lu\n", address, EXTENDED_ADDRESS);
		g_test_fail();
		return;
	}

	if (timestamp != EXTENDED_TIMESTAMP) {
		fclose(spool);
		printf("Returned value of extended 'timestamp' %lu doesn't match expected value of %lu\n", timestamp, EXTENDED_TIMESTAMP);
		g_test_fail();
		return;
	}

	if (extended_value_len != EXTENDED_VALUE_LEN) {
		fclose(spool);
		printf("Returned value of extended 'extended_value_len' %lu doesn't match expected value of %lu\n", extended_value_len, EXTENDED_VALUE_LEN);
		g_test_fail();
		return;
	}

	extended_value = malloc(extended_value_len+1); /* Null-terminate it ourself. */
	if (extended_value == NULL) {
		fclose(spool);
		printf("failed to malloc for readback of extended datapoint: %s\n", strerror(errno));
		g_test_fail();
		return;
	}
	ret = fread(extended_value, 1, extended_value_len, spool);
	if (ret != extended_value_len) {
		if (feof(spool) != 0) {
			printf("Unexpected short read of %d bytes from spool while reading back extended data point, end of file\n", ret);
		}
		if (ferror(spool) != 0) {
			printf("Unexpected short read of %d bytes from spool while reading back extended data point, there was an error\n", ret);
		}
		free(extended_value);
		fclose(spool);
		printf("Can't continue after short read\n");
		g_test_fail();
		return;
	}
	extended_value[extended_value_len] = '\0'; /* Guaranteed termination. */
	ret = strncmp(EXTENDED_VALUE, extended_value, extended_value_len);
	if (ret != 0) {
		printf("Returned value of extended 'extended_value' %s doesn't match expected value of %s\n", extended_value, EXTENDED_VALUE);
		g_test_fail();
		/* Don't need to return, fall through to regular cleanup. */
	}

	// Cleanup and finish
	free(extended_value);
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
