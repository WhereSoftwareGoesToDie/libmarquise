#include "DataFrame.pb-c.h"
#include "anchor_stats.h"
#include "frame.h"
#include "zmq_helpers.c"
#include "../config.h"

#include <unistd.h>

#include <stdlib.h>
#include <pthread.h>
#include <syslog.h>
#include <zmq.h>
#include <stdio.h>
#include <glib.h>
#include <string.h>

int as_send_frame( as_connection connection
              , DataFrame *frame) {
	char *data;
        // TODO: Serialize, compress then encrypt.
        fail_if( s_send( connection, data ) == -1
               , return -1;
               , "as_send: s_send: '%s'"
               , strerror( errno ) );
        return 0;
}

DataFrame__Tag* build_frame_tag(char* field, char* value) {
        DataFrame__Tag* tag = malloc(sizeof(DataFrame__Tag));
        int len_field = strlen(field) + 1;
        int len_value = strlen(value) + 1;
        tag->field = malloc(sizeof(char) * len_field);
        tag->value = malloc(sizeof(char) * len_value);
        strcpy(tag->field, field);
        strcpy(tag->value, value);
        return tag;
}

DataFrame* build_frame( char **tag_fields
                      , char **tag_values
                      , int tag_count
                      , uint64_t timestamp
                      , DataFrame__Type payload) {
        DataFrame *frame = malloc(sizeof(DataFrame));
        frame->timestamp = timestamp;
	return frame;
}
        

