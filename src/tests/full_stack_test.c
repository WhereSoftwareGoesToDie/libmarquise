#include <glib.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../anchor_stats.h"

typedef struct {
        as_consumer *context;
        as_connection *connection;
} fixture;

void setup( fixture *f, gconstpointer td ){
        f->context = as_consumer_new("tcp://localhost:5482", 0.1);
        g_assert( f->context );
        f->connection = as_connect(f->context);
        g_assert( f->connection );
}
void teardown( fixture *f, gconstpointer td ){
        as_close( f->connection );
        as_consumer_shutdown( f->context );
}

// The aim here is to test a bunch of messages being sent without an upstream
// connection, so many that we are forced to defer to disk to save memory.
//
// Then we bring back the upstream connection and have all of the messages come
// through intact.
void deferred_to_disk( fixture *f, gconstpointer td ){
        char *field_buf[] = {"foo"};
        char *value_buf[] = {"bar"};
        as_send_int( f->connection, field_buf, value_buf, 1, 10, 20 );
}

int main( int argc, char **argv ){
        g_test_init( &argc, &argv, NULL);
        g_test_add( "/full_stack/deferred_to_disk"
                  , fixture
                  , NULL
                  , setup
                  , deferred_to_disk
                  , teardown );
        return g_test_run();
}
