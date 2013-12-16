#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../structs.h"
#include "../defer.h"

typedef struct {
        FILE *stream;
        char *file_path;
} fixture;

void setup( fixture *f, gconstpointer td ){
        char *template = "/tmp/as_defer_file_test_XXXXXX";
        char *file_name = malloc( strlen( template ) + 1 );
        strcpy( file_name, template );
        int fd = mkstemp( file_name );
        free( file_name );
        g_assert_cmpint( fd, !=, -1 );

        f->file_path = malloc( strlen( template ) );
        strcpy( f->file_path, template );

        f->stream = fdopen( fd, "w+" );
}
void teardown( fixture *f, gconstpointer td ){
        fclose( f->stream );
        unlink( f->file_path );
}

void defer_then_read( fixture *f, gconstpointer td ){
        // set up a test burst
        data_burst *burst = malloc( sizeof( data_burst ) );
        burst->data = malloc( 5 );
        strcpy( burst->data, "hai" );
        burst->length = 4;

        as_defer_to_file( f->stream, burst );

        data_burst retrieved = as_retrieve_from_file( f->stream );

        g_assert_cmpuint( burst->length, ==, retrieved.length);
        g_assert_cmpstr( burst->data, ==, retrieved.data);
}
int main( int argc, char **argv ){
        g_test_init( &argc, &argv, NULL);
        g_test_add( "/set1/r_then_read", fixture, NULL, setup, defer_then_read, teardown );
        return g_test_run();
}
