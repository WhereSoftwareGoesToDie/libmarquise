#include "DataFrame.pb-c.h"

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
        
