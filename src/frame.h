#include "DataFrame.pb-c.h"
#include "anchor_stats.h"

/* Worker function called by the type-specific as_send_* functions in
 * anchor_stats.c. */
int as_send_frame( as_connection connection
              , DataFrame *frame);

/* Create a tag object from a field and a value. */
DataFrame__Tag* build_frame_tag(char* field, char* value);

/* Create a DataFrame object with everything set except the value itself
 * (the value is set by the type-specific functions that call
 * build_frame_skel). */
DataFrame* build_frame_skel( char **tag_fields
                      , char **tag_values
                      , int tag_count
                      , uint64_t timestamp
                      , DataFrame__Type payload);
        

