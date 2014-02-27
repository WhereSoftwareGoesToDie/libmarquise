#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>

/* Abstract out recieving internal telemetry data for profiling and
 * debugging
 */

#define TELEMETRY_OUTPUT_FILE	stderr

static inline uint64_t timestamp_now() {
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts)) return 0;
        return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

/* implements the simple non-crypto hash func by djb
 * can be used to generate tags if needed
 */
uint32_t telemetry_hash(const char *buf, size_t buflen) {
	uint32_t h = 5381;
	while (buflen)
		h += buf[--buflen];
	return h;
}

/* output to telemetry
 * whatever 'tag' means is up to the user, and can just be set to 0, but
 * was designed to be used for tracking specific objects as they head around
 * the place
 */
void telemetry_vprintf(uint32_t tag, const char *format, va_list vargs) {
	/* For now just output to stderr */
	fprintf(TELEMETRY_OUTPUT_FILE, "TTT %lu %08x ",timestamp_now(), tag);
	vfprintf(TELEMETRY_OUTPUT_FILE, format, vargs);
	fprintf(TELEMETRY_OUTPUT_FILE, "\n");
}

void telemetry_printf(uint32_t tag, const char *format, ...) {
	va_list vargs;
	va_start(vargs, format);
	telemetry_vprintf(tag, format, vargs);
	va_end(vargs);
}

/* startup and shutdown.
 * for now only gives start and stop timestamps
 */
int init_telemetry() {
	telemetry_printf(0, "Telemetry starts");
	return 0;
}

void shutdown_telemetry() {
	telemetry_printf(0, "Telemetry ends");
}
