/* This file is part of libmarquise.
 * 
 * Copyright 2014 Anchor Systems Pty Ltd and others.
 * 
 * The code in this file, and the program it is a part of, is made
 * available to you by its authors as open source software: you can
 * redistribute it and/or modify it under the terms of the BSD license.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "siphash24.h"
#include "marquise.h"

/* Write the 32-bit value in v to the byte array p. */
#define U32TO8_LE(p, v)         \
  (p)[0] = (uint8_t)((v)      ); (p)[1] = (uint8_t)((v) >>  8); \
(p)[2] = (uint8_t)((v) >> 16); (p)[3] = (uint8_t)((v) >> 24);

/* Write the 64-bit value in v to the byte array p. */
#define U64TO8_LE(p, v)         \
  U32TO8_LE((p),     (uint32_t)((v)      ));   \
U32TO8_LE((p) + 4, (uint32_t)((v) >> 32));

/* Return 1 if namespace is valid (only alphanumeric characters), otherwise
 * return 0. */
uint8_t valid_namespace(char *namespace) {
	size_t len = strlen(namespace);
	int i;
	for (i = 0; i < len; i++) {
		if (
			((namespace[i] | 32) < 'a' || (namespace[i] | 32) > 'z') && 
			(namespace[i] < '0' || namespace[i] > '9')
		) {
			return 0;
		}
	}
	return 1;
}

uint64_t marquise_hash_identifier(const unsigned char *id, size_t id_len) {
	unsigned char key[16];
	memset(key, 0, 16);
	uint64_t hash = siphash(id, id_len, key);
	// Clear the LSB because it's used for simple vs. extended metadata.
	return hash & 0x1111111111111110;
}

marquise_ctx *marquise_init(char *marquise_namespace) {
	marquise_ctx *ctx = malloc(sizeof(marquise_ctx));
	if (ctx == NULL) {
		return NULL;
	}
	if (!valid_namespace(marquise_namespace)) {
		errno = EINVAL;
		return NULL;
	}
	const char *spool_prefix = MARQUISE_SPOOL_PREFIX;
	size_t prefix_len = sizeof(MARQUISE_SPOOL_PREFIX);
	size_t ns_len = strlen(marquise_namespace);
	char *spool_path = malloc(prefix_len + ns_len + 1);
	strncpy(spool_path, spool_prefix, prefix_len);
	strncpy(spool_path+prefix_len, marquise_namespace, ns_len+1);
	ctx->spool = fopen(spool_path, "a");
	free(spool_path);
	return ctx;
}

int marquise_flush(marquise_ctx *ctx) {
	return fflush(ctx->spool);
}

int marquise_send_simple(marquise_ctx *ctx, uint64_t address, uint64_t timestamp, uint64_t value) {
	uint8_t buf[24];
	U64TO8_LE(buf, address);
	U64TO8_LE(buf+8, timestamp);
	U64TO8_LE(buf+16, value);
	if (fwrite((void*)buf, 1, 24, ctx->spool) != 24) {
		return -1;
	}
	return 0;
}

int marquise_shutdown(marquise_ctx *ctx) {
	int flush_success = marquise_flush(ctx);
	return flush_success | fclose(ctx->spool);
}
