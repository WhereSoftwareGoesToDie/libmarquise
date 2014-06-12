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

#include "siphash24.h"
#include "marquise.h"

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

uint64_t hash_identifier(const unsigned char *id, size_t id_len) {
	int i;
	unsigned char key[16];
	for (i=0; i < 16; i++) {
		key[i] = 0;
	}
	return siphash(id, id_len, key);
}

int send_simple(marquise_context *ctx, uint64_t address, uint64_t timestamp, uint64_t value) {
	return 0;
}
