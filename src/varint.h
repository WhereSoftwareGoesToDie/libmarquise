/*
 * The tl;dr on varints (full description here[0]) is that they
 * consist of an arbitrary number of bytes; at each byte, the MSB will
 * tell you if there is another byte to follow; once you've read all
 * the bytes (ignoring the MSBs), you take your list of seven-bit
 * values, reverse it (so the last-read byte is most significant),
 * concatenate everything and parse it as a standard little-endian
 * binary value. 
 * 
 * [0] https://developers.google.com/protocol-buffers/docs/encoding 
 */

#define AS_MAX_VARINT64_BYTES 9

#include <stdint.h>

/* Given a 64-bit int n and a byte buffer, fill the buffer with the
 * varint encoding of n and return the number of bytes needed (1-9, in
 * theory). Returns -1 on can't-happen. */
int encode_varint_uint64(uint64_t n, uint8_t buf[AS_MAX_VARINT64_BYTES]);
