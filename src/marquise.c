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

uint64_t marquise_hash_identifier(const char *id, size_t id_len) {
	unsigned char key[16];
	memset(key, 0, 16);
	uint64_t hash = siphash(id, id_len, key);
	return hash;
}

marquise_ctx *marquise_init(char *marquise_namespace) {
	int i;
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
	size_t spool_path_len = prefix_len + ns_len + 1 + 6 + 1;
	ctx->spool_path = malloc(spool_path_len);
	if (ctx->spool_path == NULL) {
		return NULL;
	}
	strncpy(ctx->spool_path, spool_prefix, prefix_len);
	strncpy(ctx->spool_path+prefix_len, marquise_namespace, ns_len);
	ctx->spool_path[prefix_len] = '/';
	for (i = prefix_len + 1; i < spool_path_len-1; i++) {
		ctx->spool_path[i] = 'X';
	}
	ctx->spool_path[spool_path_len-1] = '\0';
	int tmpf = mkstemp(ctx->spool_path);
	if (tmpf < 0) {
		marquise_shutdown(ctx);
		return NULL;
	}
	return ctx;
}

int marquise_send_simple(marquise_ctx *ctx, uint64_t address, uint64_t timestamp, uint64_t value) {
	ctx->spool = freopen(ctx->spool_path, "a", ctx->spool);
	if (ctx->spool == NULL) {
		return -1;
	}
	uint8_t buf[24];
	/* Set the LSB for a simple frame. */
	address &= 0x1111111111111110;
	U64TO8_LE(buf, address);
	U64TO8_LE(buf+8, timestamp);
	U64TO8_LE(buf+16, value);
	if (fwrite((void*)buf, 1, 24, ctx->spool) != 24) {
		return -1;
	}
	return fflush(ctx->spool);
}

int marquise_send_extended(marquise_ctx *ctx, uint64_t address, uint64_t timestamp, char *value, size_t value_len) {
	ctx->spool = freopen(ctx->spool_path, "a", ctx->spool);
	if (ctx->spool == NULL) {
		return -1;
	}
	size_t buf_len = 24 + value_len;
	uint8_t *buf = malloc(buf_len);
	uint64_t length_word = value_len;
	/* Set the LSB for an extended frame. */
	address |= 0x0000000000000001; 
	U64TO8_LE(buf, address);
	U64TO8_LE(buf+8, timestamp);
	U64TO8_LE(buf+16, length_word);
	memcpy(buf+24, value, value_len);
	int ret = fwrite((void*)buf, 1, buf_len, ctx->spool);
	free(buf);
	if (ret != buf_len) {
		return -1;
	}
	return fflush(ctx->spool);
}

int marquise_shutdown(marquise_ctx *ctx) {
	free(ctx->spool_path);
	return fclose(ctx->spool);
}
