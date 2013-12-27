#include <glib.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../marquise.h"

void connect_then_close() {
        // We hope that nothing is listening on this port. If anyone knows how
        // to do this better, I'd love to know.
        as_consumer consumer = as_consumer_new("tcp://localhost:6238", 0.1);
        g_assert( consumer );
        as_connection conn = as_connect( consumer );
        as_close( conn );
        as_consumer_shutdown( consumer );
}

int main( int argc, char **argv ){
        g_test_init( &argc, &argv, NULL);
        g_test_add_func( "/as_connect/connect_then_close"
                       , connect_then_close );
        return g_test_run();
}
