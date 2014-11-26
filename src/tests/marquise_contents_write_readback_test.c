#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "../marquise.h"

#define TEST_ADDRESS                1234567890123456789
#define TEST_SOURCE_DICT_SERIALISED "foo:one,bar:two,baz:three" // len=25
#define TEST_SOURCE_DICT_LENGTH     sizeof(TEST_SOURCE_DICT_SERIALISED)-1

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


void test_contents_write_readback() {
	int ret;
	char* fields[3] = { "foo", "bar", "baz" };
	char* values[3] = { "one", "two", "three" };

	// Init
	setenv("MARQUISE_SPOOL_DIR", "/tmp", 1);
	setenv("MARQUISE_LOCK_DIR", "/tmp", 1);
	marquise_ctx *ctx = marquise_init("marquisetest");
	if (ctx == NULL) {
		perror("marquise_init failed");
		g_test_fail();
		return;
	}


	// Prepare, dispatch, and cleanup the source dict
	marquise_source* test_src = marquise_new_source(fields, values, 3);
	if (test_src == NULL) {
		perror("marquise_new_source failed");
		g_test_fail();
		return;
	}

	ret = marquise_update_source(ctx, TEST_ADDRESS, test_src);
	if (ret != 0) {
		perror("marquise_update_source failed");
		g_test_fail();
		return;
	}

	marquise_free_source(test_src);


	// Finalise and save spool path
	char* written_spool_path = strdup(ctx->spool_path_contents);
	if (written_spool_path == NULL) {
		perror("failed to strdup ctx->spool_path_contents");
		g_test_fail();
		return;
	}
	ret = marquise_shutdown(ctx);
	if (ret != 0) {
		perror("marquise_shutdown failed");
		free(written_spool_path);
		g_test_fail();
		return;
	}


	// Reopen the spool file and prepare
	FILE *spool = fopen(written_spool_path, "r");
	if (spool == NULL) {
		perror("failed to reopen spool file for reading");
		free(written_spool_path);
		g_test_fail();
		return;
	}
	free(written_spool_path);


	// Now read it all back and test the serialisation
        // || address (64bit) || length (64bit) || serialised key-value pairs ||
	uint64_t address;
	uint64_t stored_address;
	uint64_t length;
	unsigned char header_buf[sizeof(address)+sizeof(length)];
	char*    source_dict;

	ret = fread(header_buf, 1, sizeof(address)+sizeof(length), spool);
	if (ret != sizeof(address)+sizeof(length)) {
		if (feof(spool) != 0) {
			printf("Unexpected short read of %d bytes from spool while reading back address and length, end of file\n", ret);
		}
		if (ferror(spool) != 0) {
			printf("Unexpected short read of %d bytes from spool while reading back address and length, there was an error\n", ret);
		}
		fclose(spool);
		printf("Can't continue after short read\n");
		g_test_fail();
		return;
	}
	LE8TOU64(address, header_buf);
	LE8TOU64(length, (header_buf+sizeof(address)));
	/* Sourcedicts are stored with the LSB set to zero. */
	stored_address = address >> 1 << 1;

	if (address != stored_address) {
		fclose(spool);
		printf("Returned value of 'address' %lu doesn't match expected value of %lu\n", address, stored_address);
		g_test_fail();
		return;
	}

	if (length != TEST_SOURCE_DICT_LENGTH) {
		fclose(spool);
		printf("Returned value of 'length' %lu doesn't match expected value of %lu\n", length, TEST_SOURCE_DICT_LENGTH);
		g_test_fail();
		return;
	}

	source_dict = malloc(length+1); /* Null-terminate it ourself. */
	if (source_dict == NULL) {
		fclose(spool);
		perror("failed to malloc for readback of source dict");
		g_test_fail();
		return;
	}
	ret = fread(source_dict, 1, length, spool);
	if (ret != length) {
		if (feof(spool) != 0) {
			printf("Unexpected short read of %d bytes from spool while reading back source dict, end of file\n", ret);
		}
		if (ferror(spool) != 0) {
			printf("Unexpected short read of %d bytes from spool while reading back source dict, there was an error\n", ret);
		}
		free(source_dict);
		fclose(spool);
		printf("Can't continue after short read\n");
		g_test_fail();
		return;
	}
	source_dict[length] = '\0'; /* Guaranteed termination. */

	if (strncmp(source_dict, TEST_SOURCE_DICT_SERIALISED, TEST_SOURCE_DICT_LENGTH) != 0) {
		printf("Readback serialised source dict '%s' doesn't match expected serialisation of '%s'\n", source_dict, TEST_SOURCE_DICT_SERIALISED);
		g_test_fail();
		/* Don't need to return, fall through to regular cleanup. */
	}

	// Cleanup and finish
	free(source_dict);
	ret = fclose(spool);
	if (ret != 0) {
		perror("failed to close spool file for reread as final action, meh");
		g_test_fail();
	}
}


int main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/marquise_contents_write_readback/contents_write_readback", test_contents_write_readback);
	return g_test_run();
}
