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
#define U32TO8_LE(p, v)                \
	(p)[0] = (uint8_t)((v));       \
	(p)[1] = (uint8_t)((v) >>  8); \
	(p)[2] = (uint8_t)((v) >> 16); \
	(p)[3] = (uint8_t)((v) >> 24);

/* Write the 64-bit value in v to the byte array p. */
#define U64TO8_LE(p, v)                            \
	U32TO8_LE((p),     (uint32_t)((v)      )); \
	U32TO8_LE((p) + 4, (uint32_t)((v) >> 32));

/* Return 1 if namespace is valid (only alphanumeric characters), otherwise
 * return 0. */
uint8_t valid_namespace(char *namespace)
{
	size_t len = strlen(namespace);
	int i;
	for (i = 0; i < len; i++) {
		if (((namespace[i] | 32) < 'a' || (namespace[i] | 32) > 'z') &&
		    (namespace[i] < '0' || namespace[i] > '9')
		    ) {
			return 0;
		}
	}
	return 1;
}

/* Return 1 if supplied source tag is valid (no colons or commas);
 * otherwise return zero.
 */
uint8_t valid_source_tag(char *tag)
{
	int i;
	for (i=0; i < strlen(tag); i++) {
		if (tag[i] == ',' || tag[i] == ':') {
			return 0;
		}
	}
	return 1;
}

/* Create a directory at path if it does not exist. Zero on success, -1 on 
 * failure. */
int mkdirp(char *path)
{
	/* Fail if we cannot create the directory for a reason other than that
	 * it already exists. */
	if (mkdir(path, 0750) && errno != EEXIST) {
		return -1;
	}
	return 0;
}

uint64_t marquise_hash_identifier(const unsigned char *id, size_t id_len)
{
	unsigned char key[16];
	memset(key, 0, 16);
	return siphash(id, id_len, key);
}

char *build_spool_path(const char *spool_prefix, char *namespace, const char* spool_type)
{
	int ret;

	const char* pathsep = "/";
	const char* new     = "new/";
	const char* tmp_tpl = "XXXXXX";

	size_t prefix_len     = strlen(spool_prefix);
	size_t ns_len         = strlen(namespace);
	size_t spool_type_len = strlen(spool_type);  /* points or contents */
	size_t new_len        = strlen(new);         /* new/    */
	size_t tmp_tpl_len    = strlen(tmp_tpl);     /* XXXXXX  */

	size_t spool_path_len =
	    prefix_len + 1 + ns_len + 1 + spool_type_len    + 1 + new_len + tmp_tpl_len + 1;
	/*               /            /   points-or-contents  /   new/      XXXXXX        \0  */

	char *spool_path = malloc(spool_path_len);
	if (spool_path == NULL) {
		return NULL;
	}
	char* spool_path_end = spool_path;

	/* Ensure the string is always null-terminated. */
	memset(spool_path, '\0', spool_path_len);

	spool_path_end = stpncpy(spool_path_end, spool_prefix, prefix_len);  /*  /prefix             */
	spool_path_end = stpncpy(spool_path_end, pathsep, 1);                /*  /prefix/            */
	spool_path_end = stpncpy(spool_path_end, namespace, ns_len);         /*  /prefix/namespace   */
	spool_path_end = stpncpy(spool_path_end, pathsep, 1);                /*  /prefix/namespace/  */

	/* Create namespace path if it doesn't exist. */
	ret = mkdirp(spool_path);
	if (ret != 0) {
		return NULL;
	}

	spool_path_end = stpncpy(spool_path_end, spool_type, spool_type_len);  /*  /prefix/namespace/{points,contents}   */
	spool_path_end = stpncpy(spool_path_end, pathsep, 1);                  /*  /prefix/namespace/{points,contents}/  */
	/* Create points/contents path if it doesn't exist. */
	ret = mkdirp(spool_path);
	if (ret != 0) {
		return NULL;
	}

	spool_path_end = stpncpy(spool_path_end, new, new_len);                /*  /prefix/namespace/{points,contents}/new/  */
	// Create new path if it doesn't exist.
	ret = mkdirp(spool_path);
	if (ret != 0) {
		return NULL;
	}

	spool_path_end = stpncpy(spool_path_end, tmp_tpl, tmp_tpl_len);        /*  /prefix/namespace/{points,contents}/new/XXXXXX  */

	int tmpf = mkstemp(spool_path);
	if (tmpf < 0) {
		free(spool_path);
		return NULL;
	}
	return spool_path;
}

marquise_ctx *marquise_init(char *marquise_namespace)
{
	marquise_ctx *ctx = malloc(sizeof(marquise_ctx));
	if (ctx == NULL) {
		return NULL;
	}
	if (!valid_namespace(marquise_namespace)) {
		errno = EINVAL;
		return NULL;
	}
	const char *spool_type_points   = "points";
	const char *spool_type_contents = "contents";

	const char *envvar_spool_prefix = getenv("MARQUISE_SPOOL_DIR");
	const char *default_spool_prefix = MARQUISE_SPOOL_DIR;
	const char *spool_prefix =
	    (envvar_spool_prefix ==
	     NULL) ? default_spool_prefix : envvar_spool_prefix;

	ctx->spool_path_points = build_spool_path(spool_prefix, marquise_namespace, spool_type_points);
	if (ctx->spool_path_points == NULL) {
		return NULL;
	}

	ctx->spool_path_contents = build_spool_path(spool_prefix, marquise_namespace, spool_type_contents);
	if (ctx->spool_path_contents == NULL) {
		free(ctx->spool_path_points);
		return NULL;
	}

	return ctx;
}

int marquise_send_simple(marquise_ctx * ctx, uint64_t address,
			 uint64_t timestamp, uint64_t value)
{
	FILE *spool = fopen(ctx->spool_path_points, "a");
	if (spool == NULL) {
		return -1;
	}
	uint8_t buf[24];
	/* Clear the LSB for a simple frame. */
	address = address >> 1 << 1;

	U64TO8_LE(buf, address);
	U64TO8_LE(buf + 8, timestamp);
	U64TO8_LE(buf + 16, value);
	if (fwrite((void *)buf, 1, 24, spool) != 24) {
		fclose(spool);
		return -1;
	}
	return fclose(spool);
}

int marquise_send_extended(marquise_ctx * ctx, uint64_t address,
			   uint64_t timestamp, char *value, size_t value_len)
{
	FILE *spool = fopen(ctx->spool_path_points, "a");
	if (spool == NULL) {
		return -1;
	}

	size_t buf_len = 24 + value_len;
	if (buf_len < value_len) {
		// Overflow
		errno = EINVAL;
		return -1;
	}

	uint8_t *buf = malloc(buf_len);
	if (buf == NULL) {
		return -1;
	}

	uint64_t length_word = value_len;
	/* Set the LSB for an extended frame. */
	address |= 1;
	U64TO8_LE(buf, address);
	U64TO8_LE(buf + 8, timestamp);
	U64TO8_LE(buf + 16, length_word);
	memcpy(buf + 24, value, value_len);
	int ret = fwrite((void *)buf, 1, buf_len, spool);
	free(buf);
	if (ret != buf_len) {
		fclose(spool);
		return -1;
	}
	return fclose(spool);
}

int marquise_shutdown(marquise_ctx * ctx)
{
	free(ctx->spool_path_points);
	free(ctx->spool_path_contents);
	free(ctx);
	return 0;
}

marquise_source *marquise_new_source(char **fields, char **values, size_t n_tags)
{
	int i;

	/* Everything looks good? Looks good. */
	for (i = 0; i < n_tags; i++) {
		if (!valid_source_tag(fields[i]) || !valid_source_tag(values[i])) {
			errno = EINVAL;
			return NULL;
		}
	}

	/* malloc all the things. */
	marquise_source *source = malloc(sizeof(marquise_source));
	if (source == NULL) {
		return NULL;
	}

	char **_fields = malloc(n_tags * sizeof(char*));
	if (fields == NULL) {
		free(source);
		return NULL;
	}

	char **_values = malloc(n_tags * sizeof(char*));
	if (values == NULL) {
		free(_fields);
		free(source);
		return NULL;
	}

	source->fields = _fields;
	source->values = _values;
	source->n_tags = n_tags;

	/* Iterate over fields,values in lock-step, malloc'ing and assigning to */
	/* fields[i] and values[i] appropriately.                               */
	/*                                                                      */
	/* XXX: strdup() is totally UTF8-clean, right? */
	for (i = 0; i < n_tags; i++) {
		/* Eugh, if these allocations fail we need to get out safely     */
		/* and free() everything before bailing out. It's dirty but I    */
		/* think this works.                                             */
		source->fields[i] = strdup(fields[i]);
		if (source->fields[i] == NULL && errno == ENOMEM) {
			break;
		}
		source->values[i] = strdup(values[i]);
		if (source->values[i] == NULL && errno == ENOMEM) {
			break;
		}
	}
	if (errno == ENOMEM) {
		/* Should only get here if we break'd. */
		marquise_free_source(source);
		return NULL;
	}

	return source;
}

void marquise_free_source(marquise_source *source)
{
	if (source != NULL) {
		int i;
		for (i = 0; i < source->n_tags; i++) {
			free(source->fields[i]);
			free(source->values[i]);
		}
		free(source->fields);
		free(source->values);
		free(source);
	}
}

int marquise_update_source(marquise_ctx *ctx, uint64_t address, marquise_source *source)
{
	// XXX: To be written. This can be considered as "submitting" the source_dict
	// off to the marquise daemon.
	return 0;
}
