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

#define MARQUISE_SPOOL_DIR "/var/spool/marquise"

typedef struct {
	char *spool_path;
	FILE *spool;
} marquise_ctx;

typedef struct {
	char **fields;
	char **values;
	size_t n_tags;
} marquise_source;

/* Return the SipHash-2-4[0] of an array of bytes, suitable to use as an 
 * address. Note that only the 63 most significant bits of this address 
 * are unique; the LSB is used as a flag for an extended datapoint.
 *
 * [0] https://131002.net/siphash/
 */
uint64_t marquise_hash_identifier(const char *id, size_t id_len);

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

/* Queue a Source (address metadata) for update. The `fields` and
 * `values` parameters are ordered lists of strings; the caller is
 * responsible for freeing. Returns zero on success and nonzero on
 * failure. 
 *
 * The comma (',') and colon (':') characters are not valid; all other
 * characters are valid.
 *
 * FIXME: does this need to be UTF-8?
 */
int marquise_update_source(marquise_ctx *ctx, uint64_t address, marquise_source *source);

/* Clean up, flush, close and free. Zero on success, nonzero on 
 * other things. */
int marquise_shutdown(marquise_ctx *ctx);	
