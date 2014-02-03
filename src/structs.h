#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <glib.h>

typedef struct {
        char *path;
        FILE *stream;
} deferral_file;

typedef struct {
        zmq_msg_t msg;
        uint64_t time_sent;
        uint16_t msg_id;
} message_in_flight;

typedef struct {
        void *client_sock;
        void *poller_sock;
        double poll_period;
} collator_args;

typedef struct {
        void *upstream_sock;
        void *collator_sock;
        deferral_file *deferral_file;
} poller_args;

typedef struct {
        char *broker;
        void *context;
        void *queue_connection;
        void *upstream_context;
        void *upstream_connection;
        deferral_file *deferral_file;
        double poll_period;
        pthread_mutex_t queue_mutex;
        pthread_mutex_t flush_mutex;
        GSList *queue;
} queue_args;

typedef struct {
        uint8_t *data;
        size_t length;
} data_burst;

typedef struct {
        uint8_t *data;
        size_t length;} frame;

