/* This file is part of libmarquise.
 * 
 * Copyright 2014 Anchor Systems Pty Ltd and others.
 * 
 * The code in this file, and the program it is a part of, is made
 * available to you by its authors as open source software: you can
 * redistribute it and/or modify it under the terms of the BSD license.
 */

#include <stdint.h>

#define MARQUISE_SPOOL_PREFIX "/var/spool/marquise/"

typedef struct {
	char *marquise_namespace;
	FILE *spool;
} marquise_ctx;

/* Return the SipHash-2-4[0] of an array of bytes, suitable to use as an 
 * address. */
uint64_t hash_identifier(const unsigned char *id, size_t id_len);

marquise_ctx *marquise_init(char *marquise_namespace);

int send_simple(marquise_ctx *ctx, uint64_t address, uint64_t timestamp, uint64_t value);
		
