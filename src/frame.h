#include "protobuf/DataFrame.pb-c.h"
#include "protobuf/DataBurst.pb-c.h"

/* Create a tag object from a field and a value. */
DataFrame__Tag build_frame_tag(char* field, char* value);

/* Create a DataFrame object with everything set except the value itself
 * (the value is set by the type-specific functions that call
 * build_frame_skel). */
DataFrame build_frame_skel( char **tag_fields
                      , char **tag_values
                      , int tag_count
                      , uint64_t timestamp
                      , DataFrame__Type payload);


/* Given an array of serialized DataFrames, return how big they're
 * expected to be when aggregated into a DataBurst by aggregate_frames.
 *
 * This function will return a value that is not less than, but may be
 * greater than, the true size (so use it for memory allocation but not
 * much else). */
size_t get_databurst_size(frame *frames, size_t count);

/* Create a DataBurst message from count byte buffers containing
 * marshalled frames. This function doesn't do any deep copies, so don't
 * free() the DataFrames.
 *
 * Takes an array of byte buffers with frames, and a byte buffer in
 * which to store  the serialized DataBurst.
 *
 * Returns the number of bytes written to burst. */
int aggregate_frames(frame *frames, size_t count, uint8_t *burst);
