#include "structs.h"
#include "macros.h"
#include "defer.h"

#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define recover_if( assertion, ... ) do {                               \
        fail_if( assertion                                              \
               , as_deferral_file_close( df );                          \
                 deferral_file *new = as_deferral_file_new();           \
                 *df = *new;                                            \
                 free( new );                                           \
               , "as_defer_to_file recovered: %s", strerror( errno ) ); \
        } while( 0 )

void as_defer_to_file( deferral_file *df, data_burst *burst ) {
        struct stat sb;
        recover_if( stat( df->path, &sb ) == -1 );
        recover_if( fseek( df->stream, 0, SEEK_END ) ) ;
        recover_if( ! fwrite( burst->data, burst->length, 1, df->stream ) );
        recover_if( ! fwrite( &burst->length, sizeof( burst->length ), 1, df->stream ) );
}

data_burst *as_retrieve_from_file( deferral_file *df ) {
        struct stat sb;
        data_burst *burst = malloc( sizeof( data_burst ) );

        // Broken file means no bursts left.
        if( stat( df->path, &sb ) == -1 )
                return NULL;

        // Empty file means no bursts left.
        fseek( df->stream, 0, SEEK_END );
        if( ftell( df->stream ) <= 0 )
                return NULL;

        // Grab the length of the burst from the end of the file
        fseek( df->stream, -( sizeof( burst->length ) ), SEEK_END );
        fread( &burst->length, sizeof( burst->length ), 1, df->stream );

        // Now read seek back that far
        fseek( df->stream
             , -( (int)burst->length + sizeof( burst->length ) )
             , SEEK_END );

        // This is where the file will be truncated.
        long chop_at = ftell( df->stream );

        burst->data = malloc( burst->length );
        fread(burst->data, 1, burst->length, df->stream);

        // And chop off the end of the file.
        ftruncate( fileno( df->stream ), chop_at );

        return burst;
}

deferral_file *as_deferral_file_new() {
        deferral_file *df = malloc( sizeof( deferral_file ) );
        char *template = "/var/tmp/as_defer_file_test_XXXXXX";
        char *file_path = malloc( strlen( template ) + 1 );
        strcpy( file_path, template );
        int fd = mkstemp( file_path );
        if ( fd == -1 ) return NULL;

        df->path = file_path;
        df->stream = fdopen( fd, "w+" );

        return df;
}

void as_deferral_file_close(deferral_file *df) {
        fclose( df->stream );
        unlink( df->path );
}
