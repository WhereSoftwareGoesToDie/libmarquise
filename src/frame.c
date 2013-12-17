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
 * - the tl;dr on varints (full description here[0]) is that they
 *   consist of an arbitrary number of bytes; at each byte, the MSB will
 *   tell you if there is another byte to follow; once you've read all
 *   the bytes (ignoring the MSBs), you take your list of seven-bit
 *   values, reverse it (so the last-read byte is most significant),
 *   concatenate everything and parse it as a standard little-endian
 *   binary value. */
char** aggregate_frames(char **frames, size_t count) {
	return NULL;
}
