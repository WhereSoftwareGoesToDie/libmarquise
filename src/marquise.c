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
#include <sys/stat.h>

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

/* Create a directory at path if it does not exist. Zero on success, -1 on 
 * failure. */
int mkdirp(char *path) {
	int ret;
	ret = mkdir(path, 0750);
	/* Fail if we cannot create the directory for a reason other than that
	 * it already exists. */
	if (ret != 0 && errno != EEXIST) {
		return -1;
	}
	return 0;
}

uint64_t marquise_hash_identifier(const char *id, size_t id_len) {
	unsigned char key[16];
	memset(key, 0, 16);
	uint64_t hash = siphash(id, id_len, key);
	return hash;
}

char *build_spool_path(const char *spool_prefix, char *namespace) {
	int i;
	int ret;
	size_t prefix_len = strlen(spool_prefix);
	size_t ns_len = strlen(namespace);
	size_t points_len = 8; /* /points/ */
	size_t new_len = 5; /* /new/ */
	size_t spool_path_len = prefix_len + ns_len + points_len + new_len + 1 + 6 + 1 + 1;
	char *spool_path = malloc(spool_path_len);
	if (spool_path == NULL) {
		return NULL;
	}
	strncpy(spool_path, spool_prefix, prefix_len);
	spool_path[prefix_len] = '/';
	strncpy(spool_path+prefix_len+1, namespace, ns_len);
	spool_path[prefix_len + ns_len+1] = '/';

	/* Temporarily null-terminate the string at the directory name 
         * so we can ensure it exists. */
	spool_path[prefix_len + ns_len+2] = '\0';
	ret = mkdirp(spool_path);
	if (ret != 0) {
		return NULL;
	}

	strcpy(spool_path + prefix_len + ns_len + 2, "/points/");
	ret = mkdirp(spool_path);
	if (ret != 0) {
		return NULL;
	}

	strcpy(spool_path + prefix_len + ns_len + 2 + points_len, "/new/");
	ret = mkdirp(spool_path);
	if (ret != 0) {
		return NULL;
	}
	
	for (i = prefix_len + ns_len + points_len + new_len + 2; i < spool_path_len-1; i++) {
		spool_path[i] = 'X';
	}
	spool_path[spool_path_len-1] = '\0';
	int tmpf = mkstemp(spool_path);
	if (tmpf < 0) {
		free(spool_path);
		return NULL;
	}
	return spool_path;
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
	const char *envvar_spool_prefix = getenv("MARQUISE_SPOOL_DIR");
	const char *default_spool_prefix = MARQUISE_SPOOL_DIR;
	const char *spool_prefix = (envvar_spool_prefix == NULL) ? default_spool_prefix : envvar_spool_prefix;
	ctx->spool_path = build_spool_path(spool_prefix, marquise_namespace);
	if (ctx->spool_path == NULL) {
		return NULL;
	}
	return ctx;
}

int marquise_send_simple(marquise_ctx *ctx, uint64_t address, uint64_t timestamp, uint64_t value) {
	FILE *spool = fopen(ctx->spool_path, "a"); 
	if (spool == NULL) {
		return -1;
	}
	uint8_t buf[24];
	/* Clear the LSB for a simple frame. */
	address &= 0xffffffffffffffff-1;
	U64TO8_LE(buf, address);
	U64TO8_LE(buf+8, timestamp);
	U64TO8_LE(buf+16, value);
	if (fwrite((void*)buf, 1, 24, spool) != 24) {
		return -1;
	}
	return fclose(spool);
}

int marquise_send_extended(marquise_ctx *ctx, uint64_t address, uint64_t timestamp, char *value, size_t value_len) {
	FILE *spool = fopen(ctx->spool_path, "a"); 
	if (spool == NULL) {
		return -1;
	}
	size_t buf_len = 24 + value_len;
	uint8_t *buf = malloc(buf_len);
	uint64_t length_word = value_len;
	/* Set the LSB for an extended frame. */
	address |= 1; 
	U64TO8_LE(buf, address);
	U64TO8_LE(buf+8, timestamp);
	U64TO8_LE(buf+16, length_word);
	memcpy(buf+24, value, value_len);
	int ret = fwrite((void*)buf, 1, buf_len, spool);
	free(buf);
	if (ret != buf_len) {
		return -1;
	}
	return fclose(spool);
}

int marquise_shutdown(marquise_ctx *ctx) {
	free(ctx->spool_path);
	return 0;
}
