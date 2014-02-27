#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>

/* Only write out telemetry if it has been started */
static volatile int telemetry_running = 0;

/* Abstract out recieving internal telemetry data for profiling and
 * debugging
 */

#define TELEMETRY_OUTPUT_FILE	stderr
#define TELEMETRY_MAX_LINE_LENGTH 1024

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
	char telem_str[TELEMETRY_MAX_LINE_LENGTH];
	if (! telemetry_running)
		return;
	vsnprintf(telem_str, TELEMETRY_MAX_LINE_LENGTH-1, format, vargs);
	telem_str[TELEMETRY_MAX_LINE_LENGTH-1] = 0;
	/* For now just output to stderr */
	fprintf(TELEMETRY_OUTPUT_FILE, "TTT %lu %08x %s\n",timestamp_now(), tag, telem_str);
}

void telemetry_printf(uint32_t tag, const char *format, ...) {
	va_list vargs;
	va_start(vargs, format);
	telemetry_vprintf(tag, format, vargs);
	va_end(vargs);
}

/* startup and shutdown.
 */
int init_telemetry() {
	telemetry_running = 1;
	return 0;
}
void shutdown_telemetry() {
	telemetry_running = 0;
}
