#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Return 1 if namespace is valid (only alphanumeric characters), otherwise
 * return 0. */
uint8_t valid_namespace(char *namespace) {
	size_t len = strlen(namespace);
	int i;
	for (i = 0; i < len; i++) {
		if (
			(namespace[i] | 32 < 'a' || namespace[i] | 32 > 'z') && 
			(namespace[i] < '0' || namespace[i] > '9')
		) {
			return 0;
		}
	}
	return 1;
}
