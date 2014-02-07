#include <glib.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../marquise.h"
#include "../protobuf/DataBurst.pb-c.h"
#include "../lz4/lz4.h"
#include <zmq.h>

typedef struct {
        marquise_consumer *context;
        marquise_connection *connection;
} fixture;

void setup( fixture *f, gconstpointer td ){
        f->context = marquise_consumer_new( "ipc:///tmp/marquise_full_stack_test", 0.1 );
        g_assert( f->context );
        f->connection = marquise_connect( f->context );
        g_assert( f->connection );
}
void teardown( fixture *f, gconstpointer td ){
        marquise_close( f->connection );
        marquise_consumer_shutdown( f->context );
}

// Starting simple, we send one message and make sure we get it.
void one_message( fixture *f, gconstpointer td ){
        char *field_buf[] = {"foo"};
        char *value_buf[] = {"bar"};

        g_assert( marquise_send_int( f->connection, field_buf, value_buf, 1, 10, 20 )
                  != -1 );

        // Now start up the server and expect them all.
        void *context = zmq_ctx_new();
        g_assert( context );
        void *bind_sock = zmq_socket( context, ZMQ_ROUTER );
        g_assert( bind_sock );
        g_assert( !zmq_bind( bind_sock, "ipc:///tmp/marquise_full_stack_test" ) );

        char *scratch = malloc(512);
        char *decompressed = malloc(512);

        // Ident
        zmq_msg_t ident;
        zmq_msg_init(&ident);
        int received = zmq_recvmsg( bind_sock, &ident, 0 );
        g_assert_cmpint( received, >, 0 );

        // Msg_id
        uint16_t msg_id;
        received = zmq_recv( bind_sock, &msg_id, 2, 0 );
        g_assert_cmpint( received, ==, sizeof( uint16_t ) );

        // Msg
        received = zmq_recv( bind_sock, scratch, 512, 0 );
        g_assert_cmpint( received, ==, 47 );
        int bytes = LZ4_decompress_safe( scratch + 8, decompressed, (47 - 8), 512 );
        g_assert_cmpint( bytes, ==, 39 );
        free( scratch );
        free( decompressed );

        g_assert( zmq_msg_send( &ident, bind_sock, ZMQ_SNDMORE ) != -1 );
        g_assert( zmq_send( bind_sock, &msg_id, sizeof(msg_id), ZMQ_SNDMORE ) != -1 );
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
        void *bind_sock = zmq_socket( context, ZMQ_ROUTER );
        g_assert( bind_sock );
        g_assert( !zmq_bind( bind_sock, "ipc:///tmp/marquise_full_stack_test" ) );


        // Send a few messages
        int i;
        char *field_buf[] = {"foo"};
        char *value_buf[] = {"bar"};
        for( i = 0; i < 8192; i++ )
                g_assert( marquise_send_int( f->connection
                                     , field_buf
                                     , value_buf
                                     , 1
                                     , 10
                                     , 20 ) != -1 );
        char *scratch = malloc( 2048 );
        char *decompressed = malloc( 3000000 );

        i = 0;
        while( i < 8192 ) {
                // Ident
                zmq_msg_t ident;
                zmq_msg_init(&ident);
                int received = zmq_recvmsg( bind_sock, &ident, 0 );
                g_assert_cmpint( received, >, 0 );

                // Msg_id
                uint16_t msg_id;
                received = zmq_recv( bind_sock, &msg_id, 2, 0 );
                g_assert_cmpint( received, ==, sizeof( uint16_t ) );

                received = zmq_recv( bind_sock, scratch, 2048, 0 );
                g_assert_cmpint(received, >, 0);

                int bytes = LZ4_decompress_safe( scratch + 8, decompressed, (received - 8), 3000000 );
                g_assert_cmpint( bytes, >, 0);
                DataBurst *burst;
                burst = data_burst__unpack( NULL
                                          , bytes
                                          , (uint8_t *)decompressed );
                g_assert( burst );

                int j;
                for ( j = 0; j < burst->n_frames; j++ ) {
                        i++;
                        g_assert_cmpint( burst->frames[j]->value_numeric, ==, 10 );
                        g_assert_cmpint( burst->frames[j]->timestamp, ==, 20 );
                }
                data_burst__free_unpacked( burst, NULL );

                g_assert( zmq_msg_send( &ident, bind_sock, ZMQ_SNDMORE ) != -1 );
                g_assert( zmq_send( bind_sock, &msg_id, sizeof(msg_id), ZMQ_SNDMORE ) != -1 );
                g_assert( zmq_send( bind_sock, "", 0, 0 ) != -1 );
        }

        free( scratch );
        free( decompressed );


        zmq_close( bind_sock );
        zmq_ctx_destroy( context );
}

int main( int argc, char **argv ){
        setenv("LIBMARQUISE_ORIGIN", "unit tests", 1);
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
        return g_test_run();
        return g_test_run();
}
