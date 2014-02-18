#include <glib.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../varint.h"

void test_varint()
{
	uint8_t buf[AS_MAX_VARINT64_BYTES];
	uint64_t n;
	int c;

	/* test with one byte */
	n = 120;
	c = encode_varint_uint64(n, buf);
	g_assert(c == 1);
	g_assert(buf[0] == 120);

	/* test with two bytes */
	n = 300;
	c = encode_varint_uint64(n, buf);
	g_assert(c == 2);
	g_assert(buf[0] == 172);
	g_assert(buf[1] == 2);

	/* test with a larger int */
	n = (1 << 30);
	c = encode_varint_uint64(n, buf);
	g_assert_cmpint(c, ==, 9);
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/encode_varint_uint64/varint", test_varint);
	return g_test_run();
}
