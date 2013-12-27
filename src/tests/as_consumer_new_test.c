#include <glib.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "../marquise.h"

void valid_arguments() {
        as_consumer c = as_consumer_new( "tcp://localhost:1234", 0.5 );
        g_assert ( c );
        as_consumer_shutdown( c );
}

void negative_poll_period() {
        g_assert( !as_consumer_new( "tcp://localhost:1234", -12 ) );
}

void unparseable_broker() {
        g_assert( !as_consumer_new( "wat?", 42) );
}


int main( int argc, char **argv ){
        g_test_init( &argc, &argv, NULL);
        g_test_add_func( "/as_consumer_new/valid"
                        , valid_arguments);
        g_test_add_func( "/as_consumer_new/negative_poll_period"
                       , negative_poll_period );
        g_test_add_func( "/as_consumer_new/unparseable_broker"
                       , unparseable_broker );
        return g_test_run();
}
