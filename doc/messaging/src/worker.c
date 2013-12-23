#include <zmq.h>
#include <assert.h>

int main( void )
{
        void *context = zmq_ctx_new();
        void *pull = zmq_socket( context, ZMQ_PULL );
        void *push = zmq_socket( context, ZMQ_PUSH );


        zmq_connect( pull, "tcp://localhost:5561" );
        zmq_connect( push, "tcp://localhost:5560" );

        int more;
        while( 1 ) {
                zmq_msg_t envelope;
                zmq_msg_init( &envelope );
                zmq_msg_recv( &envelope, pull, 0);
                assert( zmq_msg_more( &envelope ) );

                zmq_msg_t delimiter;
                zmq_msg_init( &delimiter );
                zmq_msg_recv( &delimiter, pull, 0 );
                assert( zmq_msg_more( &envelope ) );

                zmq_msg_t data_burst;
                zmq_msg_init( &data_burst );
                zmq_msg_recv( &data_burst, pull, 0);
                assert( !zmq_msg_more( &data_burst ) );

                // Do work
                printf("%s\n", zmq_msg_data( &data_burst ) );
                zmq_msg_close( &data_burst );

                zmq_msg_send( &envelope, push, ZMQ_SNDMORE );
                zmq_msg_send( &delimiter, push, ZMQ_SNDMORE );
                zmq_send( push, "ack", 3, 0 );
        }

        zmq_close( pull );
        zmq_close( push );
        zmq_ctx_destroy( context );
        return 0;
}
