#define AS_MAX_VARINT64_BYTES 9

#include <stdint.h>

/* Given a 64-bit int n and a byte buffer, fill the buffer with the
 * varint encoding of n and return the number of bytes needed (1-9, in
 * theory). Returns -1 on can't-happen. */
int encode_varint_int64(uint64_t n, uint8_t buf[AS_MAX_VARINT64_BYTES]);
