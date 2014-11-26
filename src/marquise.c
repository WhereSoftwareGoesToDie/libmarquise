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
#include <glib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>

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

/* Safe and complete destructor for marquise_ctxs */
void free_ctx(marquise_ctx *ctx) {
	if (ctx == NULL) return;
	free(ctx->marquise_namespace);
	free(ctx->spool_path_points);
	free(ctx->spool_path_contents);
	/* This avoids getting a dumb error message: */
	/* GLib-CRITICAL **: g_tree_destroy: assertion 'tree != NULL' failed */
	if (ctx->sd_hashes != NULL) {
		g_tree_destroy(ctx->sd_hashes);
	}
	free(ctx);
}

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
	uint64_t addr = siphash(id, id_len, key);
	return addr >> 1 << 1;
}

/* Build up the the folder structure for the lock file, ensuring parent folder exists.
 * Return NULL on failure
 * Return the path to the lock file on success
 */
char *build_lock_path(const char *lock_prefix, char *namespace)
{
	int ret;

	const char* pathsep = "/";
	const char* lock_ext = ".lock";

	size_t prefix_len    = strlen(lock_prefix);
	size_t namespace_len = strlen(namespace);
	size_t ext_len       = strlen(lock_ext);

	/*                     prefix       /   (namespace)     .lock     \0    */
	size_t lock_path_len = prefix_len + 1 + namespace_len + ext_len + 1;

	char *lock_path = malloc(lock_path_len);
	if (lock_path == NULL) { // error allocating memeory
		return NULL;
	}

	char* lock_path_end = lock_path;

	/* Ensure the string is always null-terminated. */
	memset(lock_path, '\0', lock_path_len);

	lock_path_end = stpncpy(lock_path_end, lock_prefix, prefix_len);    /* /prefix                */
	lock_path_end = stpncpy(lock_path_end, pathsep,     1);             /* /prefix/               */

	ret = mkdirp(lock_path);
	if (ret != 0) {
		free(lock_path);
		return NULL;
	}

	lock_path_end = stpncpy(lock_path_end, namespace,   namespace_len); /* /prefix/namspace       */
	lock_path_end = stpncpy(lock_path_end, lock_ext,    ext_len);       /* /prefix/namespace.lock */

	return lock_path;
}

/* Attempt to create and lock a file at lock_path
 * Fail if file exists with a lock (someone else is here right now) and return -1
 * Return the file descriptor on success
 */
int lock_namespace(const char *lock_path)
{
	int fd = open(lock_path, O_RDWR | O_CREAT, 0600);
	int len = (sizeof(pid_t) * 8);

	/* Attempt to lock the file. If we fail due to access issues, then we're a duplicate, error out. */
	if (flock(fd, LOCK_EX | LOCK_NB)) {
		if (errno == EACCES || errno == EAGAIN) {
			char existing_pid[len];
			read(fd, existing_pid, len);

			fprintf(stderr, "lock_namespace: %s is locked by process %s. This process will exit.\n", lock_path, existing_pid);
			return -1;
		}
	}

	/* Write the current process ID into the lockfile */
	char buf[len];
	sprintf(buf, "%d\n", getpid());

	/* Confirm the write function output (bytes written) equals what's expected */
	if (write(fd, buf, strlen(buf)) != strlen(buf)) {
		return -1;
	}

	return fd;
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
	/*                   /            /   points-or-contents  /   new/      XXXXXX        \0  */

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
	ret = mkdirp(spool_path);  /* Will return -1 on failure, with errno set to whatever mkdir(3) indicates. */
	if (ret != 0) {
		free(spool_path);
		return NULL;
	}

	spool_path_end = stpncpy(spool_path_end, spool_type, spool_type_len);  /*  /prefix/namespace/{points,contents}   */
	spool_path_end = stpncpy(spool_path_end, pathsep, 1);                  /*  /prefix/namespace/{points,contents}/  */
	/* Create points/contents path if it doesn't exist. */
	ret = mkdirp(spool_path);  /* See above for failure notes. */
	if (ret != 0) {
		free(spool_path);
		return NULL;
	}

	spool_path_end = stpncpy(spool_path_end, new, new_len);                /*  /prefix/namespace/{points,contents}/new/  */
	// Create new path if it doesn't exist.
	ret = mkdirp(spool_path);  /* See above. */
	if (ret != 0) {
		free(spool_path);
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

int maybe_rotate(marquise_ctx *ctx, spool_type t) {
	/* If the file is under max size, we're done, else rotate */
	if (t == SPOOL_POINTS) {
		if (ctx->bytes_written_points < MAX_SPOOL_FILE_SIZE) {
			return 0;
		}
	} else {
		if (ctx->bytes_written_contents < MAX_SPOOL_FILE_SIZE) {
			return 0;
		}
	}

	const char *spool_type_paths = (t == SPOOL_POINTS) ? "points" : "contents";

	const char *envvar_spool_prefix = getenv("MARQUISE_SPOOL_DIR");
	const char *default_spool_prefix = MARQUISE_SPOOL_DIR;
	const char *spool_prefix =
		(envvar_spool_prefix ==
		 NULL) ? default_spool_prefix : envvar_spool_prefix;

	char *new_spool_path = build_spool_path(spool_prefix, ctx->marquise_namespace, spool_type_paths);
	/* If new path fails to generate, keep using old one for now. */
	if (new_spool_path == NULL) {
		return -1;
	}

	if (t == SPOOL_POINTS) {
		free(ctx->spool_path_points);
		ctx->spool_path_points = new_spool_path;
		ctx->bytes_written_points = 0;
	} else {
		free(ctx->spool_path_contents);
		ctx->spool_path_contents = new_spool_path;
		ctx->bytes_written_contents = 0;
	}
	return 0;
}

/* Hash comparator for the sourcedict cache.
 * We ignore user_data. */
gint hash_comp(gconstpointer a, gconstpointer b, gpointer user_data) {
	return *(uint64_t*)a - *(uint64_t*)b;
}

marquise_ctx *marquise_init(char *marquise_namespace)
{
	marquise_ctx *ctx = malloc(sizeof(marquise_ctx));
	if (ctx == NULL) {
		return NULL;
	}

	/* Zero the struct to ensure it's clean. This allows free_ctx() to run
	 * with gay abandon, because it has no context as to which struct
	 * members have or haven't been safely defined.
	 */
	ctx->marquise_namespace = NULL;
	ctx->spool_path_points = NULL;
	ctx->spool_path_contents = NULL;
	ctx->lock_path = NULL;
	ctx->lock_fd = 0;
	ctx->sd_hashes = NULL;

	if (!valid_namespace(marquise_namespace)) {
		errno = EINVAL;
		free_ctx(ctx);
		return NULL;
	}
	ctx->marquise_namespace = strdup(marquise_namespace);
	if (ctx->marquise_namespace == NULL) {
		free_ctx(ctx);
		return NULL;
	}

	/* Create the lock for this namespace */
	const char *envvar_lock_prefix = getenv("MARQUISE_LOCK_DIR");
	const char *default_lock_prefix = MARQUISE_LOCK_DIR;
	const char *lock_prefix = (envvar_lock_prefix == NULL) ? default_lock_prefix : envvar_lock_prefix;

	ctx->lock_path = build_lock_path(lock_prefix, marquise_namespace);

	if (ctx->lock_path == NULL) {
		free_ctx(ctx);
		return NULL;
	}

	/* Exit out if namespace lock doesn't work - someone else is here */
	ctx->lock_fd = lock_namespace(ctx->lock_path);
	if (ctx->lock_fd == -1) {
		free_ctx(ctx);
		return NULL;
	}

	/* Create spool dir, et al */
	const char *envvar_spool_prefix = getenv("MARQUISE_SPOOL_DIR");
	const char *default_spool_prefix = MARQUISE_SPOOL_DIR;
	const char *spool_prefix =
		(envvar_spool_prefix ==
		 NULL) ? default_spool_prefix : envvar_spool_prefix;

	ctx->spool_path_points = build_spool_path(spool_prefix, marquise_namespace, "points");
	if (ctx->spool_path_points == NULL) {
		free_ctx(ctx);
		return NULL;
	}

	ctx->spool_path_contents = build_spool_path(spool_prefix, marquise_namespace, "contents");
	if (ctx->spool_path_contents == NULL) {
		free_ctx(ctx);
		return NULL;
	}
	ctx->bytes_written_points = 0;
	ctx->bytes_written_contents = 0;
	ctx->sd_hashes = g_tree_new_full(hash_comp, NULL, free, free);
	return ctx;
}

/* Writes the context of buf (representing an already-serialized
 * datapoint, either simple or extended) to the current spool file.
 * If (post-write) the amount of data we've written to the current spool
 * file exceeds MAX_SPOOL_FILE_SIZE, set a new spool file as current for
 * next time.
 *
 * Returns zero on success, -1 on error, or panics and exits with status
 * 1 if passed an invalid spool type.
 */
int rotating_write(marquise_ctx * ctx, uint8_t *buf, size_t buf_size, spool_type t) {
	char* spool_path = (t == SPOOL_POINTS) ? ctx->spool_path_points : ctx->spool_path_contents ;
	FILE *spool = fopen(spool_path, "a");
	if (spool == NULL) {
		return -1;
	}
	if (fwrite((void *)buf, 1, buf_size, spool) != buf_size) {
		fclose(spool);
		return -1;
	}
	if (t == SPOOL_POINTS) {
		ctx->bytes_written_points += buf_size;
	} else if (t == SPOOL_CONTENTS) {
		ctx->bytes_written_contents += buf_size;
	} else {
		/* We were passed an invalid spool_type, shouldn't ever
		 * happen as this function isn't exposed. */
		fprintf(stderr, "rotating_write: passed an invalid spool type %d, this can't happen. Please report a bug.\n", t);
		fclose(spool);
		exit(EXIT_FAILURE);
	}
	maybe_rotate(ctx, t);
	return fclose(spool) ? -1 : 0;
}

int marquise_send_simple(marquise_ctx * ctx, uint64_t address,
			 uint64_t timestamp, uint64_t value)
{
	uint8_t buf[24];
	/* Clear the LSB for a simple frame. */
	address = address >> 1 << 1;

	U64TO8_LE(buf, address);
	U64TO8_LE(buf + 8, timestamp);
	U64TO8_LE(buf + 16, value);
	return rotating_write(ctx, buf, 24, SPOOL_POINTS);
}

int marquise_send_extended(marquise_ctx * ctx, uint64_t address,
			   uint64_t timestamp, char *value, size_t value_len)
{
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
	int ret = rotating_write(ctx, buf, buf_len, SPOOL_POINTS);
	free(buf);
	return ret;
}

int marquise_shutdown(marquise_ctx * ctx)
{
	int ret = fcntl(ctx->lock_fd, F_GETFD);
	if (ret > 0) {
		flock(ctx->lock_fd, LOCK_UN);
	}

	if (access(ctx->lock_path, F_OK) != -1) {
		unlink(ctx->lock_path);
	}

	free_ctx(ctx);
	return 0;
}

marquise_source *marquise_new_source(char **fields, char **values, size_t n_tags)
{
	int i;

	/* We will test for ENOMEM later. */
	errno = 0;

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

/* Takes a marquise_source and serialises it to a string, suitable to append
 * to a contents spool file. It is the caller's responsibility to free() the
 * serialised string once it is no longer needed.
 *
 * XXX: This is full of assumptions that fields and values are properly
 * null-terminated.
 */
char *serialise_marquise_source(marquise_source *source)
{
	int i;
	const char* keyvalsep     = ":";
	const char* keyvalpairsep = ",";

	/* Measure the size of the source_dict */
	int dict_size_accumulator = 0;
	for (i = 0; i < source->n_tags; i++) {
		/* Add 1 for the key:value separating colon. */
		dict_size_accumulator += strlen(source->fields[i]) + strlen(source->values[i]) + 1;
	}
	/* There's a comma between each key-value pair. */
	dict_size_accumulator += source->n_tags - 1;
	/* One more for the terminating null byte. */
	dict_size_accumulator++;

	/* Go ahead and allocate. */
	char* serialised_dict = malloc(dict_size_accumulator);
	if (serialised_dict == NULL) {
		return NULL;
	}
	char* serialised_dict_end = serialised_dict;
	/* Ensure the string is always null-terminated. */
	memset(serialised_dict, '\0', dict_size_accumulator);

	/* Serialise the source_dict into serialised_dict. Like this:  k1:v1,k2:v2,k3:v3 */
	for (i = 0; i < source->n_tags; i++) {
		serialised_dict_end = stpncpy(serialised_dict_end, source->fields[i], strlen(source->fields[i]));
		serialised_dict_end = stpncpy(serialised_dict_end, keyvalsep, 1);
		serialised_dict_end = stpncpy(serialised_dict_end, source->values[i], strlen(source->values[i]));

		/* Don't add a pair-separator after the last key-value pair. */
		if (i < source->n_tags-1) {
			serialised_dict_end = stpncpy(serialised_dict_end, keyvalpairsep, 1);
		}
	}

	return serialised_dict;
}

int marquise_update_source(marquise_ctx *ctx, uint64_t address, marquise_source *source)
{
	/* Appends the source_dict to the spool_path_contents file.

	Data structure written to spool file:
	|| address (64bit) || length (64bit) || serialised key-value pairs ||
	*/
	uint64_t serialised_dict_len;
	size_t   buf_len;
	size_t   header_size = sizeof(address) + sizeof(serialised_dict_len);

	char* serialised_dict = serialise_marquise_source(source);
	if (serialised_dict == NULL) {
		return -1;
	}

	uint64_t *hash = malloc(sizeof(uint64_t));
	serialised_dict_len = strlen(serialised_dict);
	*hash = marquise_hash_identifier((const unsigned char*)serialised_dict, serialised_dict_len);

	/* If hash is not present in the cache, add and continue, else early exit*/
	if (g_tree_lookup(ctx->sd_hashes, (gpointer)hash) == NULL) {
		int *dummy_value = malloc(sizeof(int)); //Dummy value, could be anything not NULL
		*dummy_value = 1;
		g_tree_insert(ctx->sd_hashes, (gpointer)hash, (gpointer)dummy_value);
	} else {
		free(serialised_dict);
		free(hash);
		return 0;
	}

	/* Get sizes and sanity check our measurements. */
	buf_len = header_size + serialised_dict_len;
	if (buf_len < serialised_dict_len) {
		// 0verflow
		free(serialised_dict);
		errno = EINVAL;
		return -1;
	}

	/* Looks safe to proceed. */
	uint8_t *buf = malloc(buf_len);
	if (buf == NULL) {
		free(serialised_dict);
		return -1;
	}

	/* Fix the address, all sourcedicts have the LSB set to zero. */
	address = address >> 1 << 1;

	/* Build the buffer. */
	U64TO8_LE(buf, address);
	U64TO8_LE(buf + sizeof(address), serialised_dict_len);
	memcpy(buf + header_size, serialised_dict, serialised_dict_len);
	free(serialised_dict);

	/* Write it out, we're done. */
	int ret = rotating_write(ctx, buf, buf_len, SPOOL_CONTENTS);
	free(buf);
	return ret;
}
