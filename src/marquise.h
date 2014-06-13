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

#define MARQUISE_SPOOL_PREFIX "/var/spool/marquise/"

typedef struct {
	char *marquise_namespace;
	FILE *spool;
} marquise_ctx;

/* Return the SipHash-2-4[0] of an array of bytes, suitable to use as an 
 * address. 
 *
 * [0] https://131002.net/siphash/
 */
uint64_t marquise_hash_identifier(const char *id, size_t id_len);

/* Initialize the marquise context. Namespace must be unique on the 
 * current host, and alphanumeric. Returns NULL on failure. */
marquise_ctx *marquise_init(char *marquise_namespace);

/* Queue a simple datapoint (i.e., a 64-bit word) to be sent by 
 * the Marquise daemon. Returns zero on success and nonzero on 
 * failure. */
int marquise_send_simple(marquise_ctx *ctx, uint64_t address, uint64_t timestamp, uint64_t value);

/* Queue an extended datapoint (i.e., a string) to be sent by the 
 * Marquise daemon. Returns zero on success and nonzero on failure. */
int marquise_send_extended(marquise_ctx *ctx, uint64_t address, uint64_t timestamp, char *value, size_t value_len);

/* Ensure the spool file is flushed to disk. You shouldn't normally 
 * need to call this. Return value is the same as fflush. */
int marquise_flush(marquise_ctx *ctx);	

/* Clean up, flush, close and free. Zero on success, nonzero on 
 * other things. */
int marquise_shutdown(marquise_ctx *ctx);	
