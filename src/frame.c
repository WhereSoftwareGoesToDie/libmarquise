#include "protobuf/DataFrame.pb-c.h"
#include "protobuf/DataBurst.pb-c.h"
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

DataFrame__Tag build_frame_tag(char* field, char* value) {
        DataFrame__Tag tag = DATA_FRAME__TAG__INIT;
        int len_field = strlen(field) + 1;
        int len_value = strlen(value) + 1;
        tag.field = malloc(sizeof(char) * len_field);
        tag.value = malloc(sizeof(char) * len_value);
        strcpy(tag.field, field);
        strcpy(tag.value, value);
        return tag;
}

DataFrame build_frame_skel( char **tag_fields
                           , char **tag_values
                           , int tag_count
                           , uint64_t timestamp
                           , DataFrame__Type payload) {
	int i;
        DataFrame frame = DATA_FRAME__INIT;
	frame.n_source = tag_count;
	frame.source = malloc(sizeof(DataFrame__Tag*) * tag_count);
	for (i = 0; i < tag_count; ++i) {
		frame.source[i] = malloc(sizeof(DataFrame__Tag));
		*(frame.source[i]) = build_frame_tag(tag_fields[i], tag_values[i]);
	}
	frame.payload = payload;
        frame.timestamp = timestamp;
	return frame;
}

/* TODO: actually compute these values
 * 
 *       (for now we just use AS_MAX_VARINT64_SIZE for all varints) */
size_t get_databurst_size(uint8_t **frames, size_t *lengths, size_t count) {
	int burst_size;
	int i;
	/* one for the known byte 0x0a, the rest for the frame-size encoding */
	burst_size = 1 + AS_MAX_VARINT64_SIZE; 
	/* plus the actual sizes of the frames, of course */
	for (i = 0; i < count; i++) {
		burst_size += lengths[i];
	}
	return burst_size;
}

/* Herein we turn an array of byte buffers representing DataFrames into
 * a single DataBurst, then marshal it to another char**. 
 * 
 * We do this via protobuf tomfoolery:
 * 
 *  - on the wire, each DataFrame is preceded by two variable-length
 *    values a and b, such that
 *    
 *  - a is a varint representing ((field_number << 3) | wire_type)
 * 
 *   - field_number is 1 
 *   - wire_type is 2 (for length-delimited)
 *   - a is therefore ((1 << 3) | 2) (or 0x0a, or ten). 
 * 
 * - b is simply the size (in bytes) of the following frame
 *   representation, encoded as a varint.
 *
 * - see varint.h for more on that topic. */
int aggregate_frames(uint8_t **frames, size_t *frame_lengths, uint8_t *burst, size_t count) {
       	 
        return NULL;
}
