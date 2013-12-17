#include <varint.h>

/* This is a lazy hack, but we only need to specify frame lengths with
 * these. 
 *
 * Works like this: for each i = 1 <= n <= 9, we ask if that number is less
 * than 2**(7*i). 
 *
 * If yes, we set buf[i-1] to the relevant subset of
 * bits (8*(i-1) to 8*(i-1) + 7) (note that this leaves the MSB as 0). 
 *
 * If no, we do the same thing but with the MSB set to 1 to indicate
 * that there is another byte to follow. */
int encode_varint_uint64(uint64_t n, uint8_t buf[AS_MAX_VARINT64_BYTES]) {
	int i;
	for (i = 1; i <= 9; i++) {
		if (n < (1 << (7*i))) {
			buf[i-1] = (uint8_t) (n >> 7*(i-1));
			return i;
		}
		buf[i-1] = (uint8_t) ((n >> 7*(i-1)) | (1 << 7));
	}
	/* this can't happen */
	return -1;
}
	
