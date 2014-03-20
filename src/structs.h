#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <glib.h>
#include <zmq.h>

#ifndef H_MARQUISE_STRUCTS
#define H_MARQUISE_STRUCTS

typedef struct {
	char *path;
	FILE *stream;
	int fd;
} deferral_file;

typedef struct {
	zmq_msg_t msg;
	uint64_t expiry;
	uint16_t msg_id;
} message_in_flight;

typedef struct {
	void *client_sock;
	void *poller_sock;
	void *ipc_event_sock;
	double poll_period;
} collator_args;

typedef struct {
	void *upstream_sock;
	void *collator_sock;
	deferral_file *deferral_file;
} poller_args;

typedef struct {
	uint8_t *data;
	size_t length;
} data_burst;

typedef struct {
	uint8_t *data;
	size_t length;
} frame;

#endif
