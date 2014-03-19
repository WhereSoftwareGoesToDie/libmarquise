/* telemetry for marquise
 *
 * The intent of telemetry is to make it as easy as possible for 
 * the caller to log machine readable information wherever needed
 * inside code, without having to clutter the code with specific
 * state.
 *
 * It is now possible to send telemetry upstream to the broker
 * via PUB/SUB to get an idea of what the whole system is doing as
 * a whole (e.g. to get global points outstanding)
 */

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>

#include <zmq.h>

#include "telemetry.h"

/* Flag to show if telemetry is running or not */
static volatile int telemetry_running = 0;

/* zmq context for all telemetry */
static volatile void *telemetry_context = NULL;

/* per-thread telemetry socket to send inproc to the aggregator */
static __thread void *thread_telemetry_sock = NULL;

/* A file to telemetry to write to if not NULL */
static volatile FILE *telemetry_output_file = NULL;

static volatile char *telemetry_client_name;

#define INTERNAL_TELEMETRY_URI	"inproc://marquise_telemetry/"
#define TELEMETRY_MAX_LINE_LENGTH 1024

#define __FENCE__ __atomic_signal_fence(__ATOMIC_SEQ_CST); __atomic_thread_fence(__ATOMIC_SEQ_CST);
#define __FENCED__(_FENCED_BLOCK) {  __FENCE__ { _FENCED_BLOCK } __FENCE__ }


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
		h = (h<<5) + h + buf[--buflen];
	return h;
}

/* Send a string out the telemetry pub/sub
 * If there isn't currently a socket open to the internal
 * aggregator for this thread, it opens one first
 */
static void telemetry_zmq_send(char *buf, size_t buflen) {
	if (thread_telemetry_sock == NULL) {
		/* Connect to the internal PUB/SUB aggregator */
		thread_telemetry_sock = zmq_socket((void *)telemetry_context, ZMQ_PUB);
		if (thread_telemetry_sock == NULL) {
			perror("zmq_socket");
			return;
		}
		if (zmq_connect(thread_telemetry_sock, INTERNAL_TELEMETRY_URI)) {
			zmq_close(thread_telemetry_sock);
			thread_telemetry_sock = NULL;
			perror("zmq_connect");
			return;
		}
	}
	assert(thread_telemetry_sock != NULL);
	if (zmq_send(thread_telemetry_sock, buf, buflen, 0) != buflen)
		perror("zmq_send");
}

/* output to telemetry
 * whatever 'tag' means is up to the user, and can just be set to 0, but
 * was designed to be used for tracking specific objects as they head around
 * the place
 */
void telemetry_vprintf(uint32_t tag, const char *format, va_list vargs) {
	char telem_str[TELEMETRY_MAX_LINE_LENGTH];
	char annotated_telem_str[TELEMETRY_MAX_LINE_LENGTH];
	if (! telemetry_running)
		return;

	/* Build the string for the supplied stuff */
	vsnprintf(telem_str, TELEMETRY_MAX_LINE_LENGTH-1, format, vargs);
	telem_str[TELEMETRY_MAX_LINE_LENGTH-1] = 0;

	/* Add our own stuff to give context */
	snprintf(annotated_telem_str, TELEMETRY_MAX_LINE_LENGTH-1,
			"%s %lu %08x %s", telemetry_client_name, timestamp_now(), tag, telem_str);
	annotated_telem_str[TELEMETRY_MAX_LINE_LENGTH-1] = 0;

	/* Output */
	if (telemetry_output_file != NULL)
		fprintf((FILE *)telemetry_output_file, "%s\n", annotated_telem_str);

	if (telemetry_context != NULL)
		telemetry_zmq_send(annotated_telem_str, strlen(annotated_telem_str));
}


void telemetry_printf(uint32_t tag, const char *format, ...) {
	va_list vargs;
	va_start(vargs, format);
	telemetry_vprintf(tag, format, vargs);
	va_end(vargs);
}


void * telemetry_thread(void *arg)
{
	const char *broker_telemetry_uri = (const char *)arg;
	void *xsub_socket = zmq_socket((void *)telemetry_context, ZMQ_XSUB);
	void *xpub_socket = zmq_socket((void *)telemetry_context, ZMQ_XPUB);

	zmq_bind(xsub_socket, INTERNAL_TELEMETRY_URI);
	zmq_connect(xpub_socket, broker_telemetry_uri);

	__FENCED__(
		telemetry_running = 1;
	);

	zmq_proxy(xsub_socket,xpub_socket, NULL);
	zmq_close(xsub_socket);
	zmq_close(xpub_socket);
	return NULL;
}

/* startup and shutdown.
 *
 * This is called once per process, and sets up the telemetry proxy thread etc.
 * It is not re-enterant
 * 
 * If telemetry_output_file is not NULL, telemetry will be sent to the file
 * If telemetry_broker_uri is not NULL, telemetry will be sent via the broker
 *
 * returns 0 on success
 * returns -1 on failure and sets errno
 */
static pthread_t telemetry_pthread;
int init_telemetry(FILE *telemetryf, char *telemetry_broker_uri, char *client_name) {
	if (telemetryf == NULL && telemetry_broker_uri == NULL) {
		errno = EINVAL;  /* Need to supply at least one output */
		return -1;
	}

	if (client_name) {
		telemetry_client_name = client_name;
	} else {
		telemetry_client_name = malloc(512);
		if (telemetry_client_name == NULL) {
			telemetry_client_name = "marquise";
		} else {
			gethostname((char *)telemetry_client_name, 512);
			telemetry_client_name[511] = 0;
		}
	}

	if (telemetryf) {
		setvbuf(stderr, NULL, _IOLBF, 0);
		telemetry_output_file = telemetryf;
	}

	if (telemetry_broker_uri) {
		telemetry_context = zmq_ctx_new();
		if (pthread_create(&telemetry_pthread, NULL, telemetry_thread, telemetry_broker_uri)) {
			zmq_ctx_destroy((void *)telemetry_context);
			return -1;
		}
	}
	else {
		/* If the 0mq thread isn't being run we have to set the
		 * running flag ourselves
		*/	
		__FENCED__( 
			telemetry_running = 1; 
		);
	}
	return 0;
}
void shutdown_telemetry() {
	/* Stop more stuff being sent into the telemetry queue */
	__FENCED__(
		telemetry_running = 0;
	)
	if (telemetry_context != NULL) {
		zmq_ctx_shutdown((void *)telemetry_context);
		/* FIXME: This will block unless we close all sockets, but we can't do that
		   from a single thread, and we currently leave sockets open once connected
		   so we don't have to open a new one for every telemetry message
		*/
	//	zmq_ctx_term((void *)telemetry_context);
	}
	__FENCED__(
		telemetry_context = NULL;
		telemetry_output_file = NULL;
	)
}
