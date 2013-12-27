#include <glib.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../marquise.h"
#include <zmq.h>

typedef struct {
        as_consumer *context;
        as_connection *connection;
} fixture;

void setup( fixture *f, gconstpointer td ){
        f->context = as_consumer_new("ipc:///tmp/as_full_stack_test", 0.1);
        g_assert( f->context );
        f->connection = as_connect(f->context);
        g_assert( f->connection );
}
void teardown( fixture *f, gconstpointer td ){
        as_close( f->connection );
        as_consumer_shutdown( f->context );
}

// Starting simple, we send one message and make sure we get it.
void one_message( fixture *f, gconstpointer td ){
        char *field_buf[] = {"foo"};
        char *value_buf[] = {"bar"};

        g_assert( as_send_int( f->connection, field_buf, value_buf, 1, 10, 20 )
                  != -1 );

        // Now start up the server and expect them all.
        void *context = zmq_ctx_new();
        g_assert( context );
        void *bind_sock = zmq_socket( context, ZMQ_REP );
        g_assert( bind_sock );
        g_assert( !zmq_bind( bind_sock, "ipc:///tmp/as_full_stack_test" ) );

        char *scratch = malloc(512);
        int recieved = zmq_recv( bind_sock, scratch, 512, 0 );
        g_assert_cmpint( recieved, ==, 27 );
        free( scratch );
        g_assert( zmq_send( bind_sock, "", 0, 0 ) != -1 );

        zmq_close( bind_sock );
        zmq_ctx_destroy( context );
}

// Now send a bunch, we should get them all in one data-burst.
void many_messages( fixture *f, gconstpointer td ){
        // This time we start up the server first, to ensure there's no
        // difference there.
        void *context = zmq_ctx_new();
        g_assert( context );
        void *bind_sock = zmq_socket( context, ZMQ_REP );
        g_assert( bind_sock );
        g_assert( !zmq_bind( bind_sock, "ipc:///tmp/as_full_stack_test" ) );


        // Send a few messages
        int i;
        char *field_buf[] = {"foo"};
        char *value_buf[] = {"bar"};
        for( i = 0; i < 8192; i++ )
                g_assert( as_send_int( f->connection
                                     , field_buf
                                     , value_buf
                                     , 1
                                     , 10
                                     , 20 ) != -1 );

        char *scratch = malloc(512);
        int recieved = zmq_recv( bind_sock, scratch, 512, 0 );
        g_assert_cmpint( recieved, ==, 221184 );
        free( scratch );
        g_assert( zmq_send( bind_sock, "", 0, 0 ) != -1 );

        zmq_close( bind_sock );
        zmq_ctx_destroy( context );
}

static void *server( void *args ) {
        void *context = zmq_ctx_new();
        g_assert( context );
        void *bind_sock = zmq_socket( context, ZMQ_REP );
        g_assert( bind_sock );
        g_assert( !zmq_bind( bind_sock, "ipc:///tmp/as_full_stack_test" ) );

        char *scratch = malloc(512);
        int recieved = zmq_recv( bind_sock, scratch, 512, 0 );
        g_assert_cmpint( recieved, ==, 27 );
        free( scratch );
        g_assert( zmq_send( bind_sock, "", 0, 0 ) != -1 );

        zmq_close( bind_sock );
        zmq_ctx_destroy( context );
}

void defer_to_disk( fixture *f, gconstpointer td ){
        char *field_buf[] = {"foo"};
        char *value_buf[] = {"bar"};

        // Send one message
        g_assert( as_send_int( f->connection, field_buf, value_buf, 1, 10, 20 )
                  != -1 );

        // Now sleep till the file is deferred to disk at least once. This
        // should cause some syslog errors.
        sleep( 4 );

        // Start a server in a new thread.
        pthread_t server_thread;
        g_assert( !pthread_create( &server_thread
                                 , NULL
                                 , server
                                 , NULL ) );


        // And shutdown the connection immediately.
        as_close( f->connection );
        as_consumer_shutdown( f->context );
}

int main( int argc, char **argv ){
        g_test_init( &argc, &argv, NULL);
        g_test_add( "/full_stack/one_message"
                  , fixture
                  , NULL
                  , setup
                  , one_message
                  , teardown );
        g_test_add( "/full_stack/many_messages"
                  , fixture
                  , NULL
                  , setup
                  , many_messages
                  , teardown );
        g_test_add( "/full_stack/defer_to_disk"
                  , fixture
                  , NULL
                  , setup
                  , defer_to_disk
                  , NULL );
        return g_test_run();
        return g_test_run();
}
