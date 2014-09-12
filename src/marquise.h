/* This file is part of libmarquise.
 * 
 * Copyright 2014 Anchor Systems Pty Ltd and others.
 * 
 * The code in this file, and the program it is a part of, is made
 * available to you by its authors as open source software: you can
 * redistribute it and/or modify it under the terms of the BSD license.
 */

#include <stdint.h>
#include <stdio.h>
#include <glib.h>

#define MARQUISE_SPOOL_DIR "/var/spool/marquise"
#define MAX_SPOOL_FILE_SIZE 1024*1024

#define SPOOL_POINTS   0
#define SPOOL_CONTENTS 1

typedef int spool_type;

typedef struct {
	char *marquise_namespace;
	char *spool_path[2];
	size_t bytes_written[2];
	GTree *sd_hashes;
} marquise_ctx;

typedef struct {
	char **fields;
	char **values;
	size_t n_tags;
} marquise_source;

/* Creates a Source from an ordered list of field names and an ordered
 * list of values. Returns NULL on error.
 *
 * The comma (',') and colon (':') characters are not valid; all other
 * characters are valid. errno is set to EINVAL on invalid input.
 *
 * FIXME: does this need to be UTF-8?
 */
marquise_source *marquise_new_source(char **fields, char **values, size_t n_tags);

void marquise_free_source(marquise_source *source);

/* Return the SipHash-2-4[0] of an array of bytes, suitable to use as an 
 * address. Note that only the 63 most significant bits of this address 
 * are unique; the LSB is used as a flag for an extended datapoint.
 *
 * [0] https://131002.net/siphash/
 */
uint64_t marquise_hash_identifier(const unsigned char *id, size_t id_len);

/* Initialize the marquise context. Namespace must be unique on the 
 * current host, and alphanumeric. Returns NULL on failure. 
 * 
 * marquise_init will open a file in the SPOOL_DIR directory; this will
 * default to "/var/spool/marquise", but can be overridden by the
 * MARQUISE_SPOOL_DIR environment variable.
 */
marquise_ctx *marquise_init(char *marquise_namespace);

/* Queue a simple datapoint (i.e., a 64-bit word) to be sent by 
 * the Marquise daemon. Returns zero on success and nonzero on 
 * failure. */
int marquise_send_simple(marquise_ctx *ctx, uint64_t address, uint64_t timestamp, uint64_t value);

/* Queue an extended datapoint (i.e., a string) to be sent by the 
 * Marquise daemon. Returns zero on success and nonzero on failure. */
int marquise_send_extended(marquise_ctx *ctx, uint64_t address, uint64_t timestamp, char *value, size_t value_len);

/* Queue a Source (address metadata) for update. The caller is
 * responsible for freeing the source (using `marquise_free_source`).
 * Returns zero on success, nonzero on failure.
 */
int marquise_update_source(marquise_ctx *ctx, uint64_t address, marquise_source *source);

/* Clean up, flush, close and free. Zero on success, nonzero on 
 * other things. */
int marquise_shutdown(marquise_ctx *ctx);
