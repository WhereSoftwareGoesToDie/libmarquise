#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "../marquise.h"

void test_hash_identifier() {
	const char *id = "hostname:fe1.example.com,metric:BytesUsed,service:memory,";
	size_t id_len = strlen(id);
	uint64_t address = marquise_hash_identifier((const unsigned char*) id, id_len);
	g_assert_cmpint(address, ==, 7602883380529707052);
}

void test_hash_clear_lsb() {
	const char *id = "bytes:tx,collection_point:syd1,ip:110.173.152.33,";
	size_t id_len = strlen(id);
	uint64_t address = marquise_hash_identifier((const unsigned char*) id, id_len);
	g_assert_cmpint(address, ==, -8873247187777138600);
}

int main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/marquise_hash_identifier/hash", test_hash_identifier);
	g_test_add_func("/marquise_hash_identifier/clear_lsb", test_hash_clear_lsb);
	return g_test_run();
}
