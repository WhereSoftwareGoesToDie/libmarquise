#include <stdlib.h>
#include <stdio.h>

typedef struct {
        void *context;
        void *queue_connection;
        void *upstream_connection;
        FILE *defer_stream;
        double poll_period;
} queue_args;

typedef struct {
        char *data;
        size_t length;
} data_burst;
