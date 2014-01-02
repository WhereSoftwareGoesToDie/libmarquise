#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <glib.h>

typedef struct {
        char *path;
        FILE *stream;
} deferral_file;

typedef struct {
        char *broker;
        void *context;
        void *queue_connection;
        void *upstream_context;
        void *upstream_connection;
        deferral_file *deferral_file;
        double poll_period;
        pthread_mutex_t queue_mutex;
        GSList *queue;
} queue_args;

typedef struct {
        uint8_t *data;
        size_t length;
} data_burst;

typedef struct {
        uint8_t *data;
        size_t length;
} frame;

