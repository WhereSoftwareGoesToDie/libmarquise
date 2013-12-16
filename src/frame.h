#include "DataFrame.pb-c.h"
#include "anchor_stats.h"

int as_send_frame( as_connection connection
              , DataFrame *frame);

DataFrame__Tag* build_frame_tag(char* field, char* value);

DataFrame* build_frame( char **tag_fields
                      , char **tag_values
                      , int tag_count
                      , uint64_t timestamp
                      , DataFrame__Type payload);
        

