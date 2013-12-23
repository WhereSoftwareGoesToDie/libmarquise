#include <zmq.h>
#include <stdio.h>
#include <assert.h>

int main( void )
{
        void *context = zmq_ctx_new();
        void *req = zmq_socket( context, ZMQ_REQ );

        zmq_connect( req, "tcp://127.0.0.1:5559" );

        int more;
        while( 1 ) {
                zmq_send( req, "can has ack?", 12, 0 );

                zmq_msg_t response;
                zmq_msg_init( &response );
                zmq_msg_recv( &response, req, 0 );
                printf("%s\n", zmq_msg_data( &response ) );
                zmq_msg_close( &response );
        }

        zmq_close( req );
        zmq_ctx_destroy( context );
        return 0;
}
