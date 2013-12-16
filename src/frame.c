#include "DataFrame.pb-c.h"
#include "anchor_stats.h"
#include "frame.h"
#include "../config.h"

#include <unistd.h>

#include <stdlib.h>
#include <pthread.h>
#include <syslog.h>
#include <zmq.h>
#include <stdio.h>
#include <glib.h>
#include <string.h>

DataFrame__Tag* build_frame_tag(char* field, char* value) {
        DataFrame__Tag* tag = DATA_FRAME__TAG__INIT;
        int len_field = strlen(field) + 1;
        int len_value = strlen(value) + 1;
        tag->field = malloc(sizeof(char) * len_field);
        tag->value = malloc(sizeof(char) * len_value);
        strcpy(tag->field, field);
        strcpy(tag->value, value);
        return tag;
}

DataFrame* build_frame_skel( char **tag_fields
                           , char **tag_values
                           , int tag_count
                           , uint64_t timestamp
                           , DataFrame__Type payload) {
	int i;
        DataFrame *frame = DATA_FRAME__INIT;
	frame->n_source = tag_count;
	frame->source = malloc(sizeof(DataFrame__Tag) * tag_count);
	for (i = 0; i < tag_count; ++i) {
		frame->source[i] = build_frame_tag(tag_fields[i], tag_values[i]);
	}
	frame->payload = payload;
        frame->timestamp = timestamp;
	return frame;
}
        

