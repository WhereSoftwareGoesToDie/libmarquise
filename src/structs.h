#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef struct {
        char *path;
        FILE *stream;
} deferral_file;

typedef struct {
        void *context;
        void *queue_connection;
        void *upstream_connection;
        deferral_file *deferral_file;
        double poll_period;
} queue_args;

typedef struct {
        uint8_t *data;
        size_t length;
} data_burst;

