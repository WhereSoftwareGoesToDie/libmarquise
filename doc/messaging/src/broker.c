#include <zmq.h>

int main( void )
{
        void *context = zmq_ctx_new();
        void *router = zmq_socket( context, ZMQ_ROUTER );
        void *push = zmq_socket( context, ZMQ_PUSH );
        void *pull = zmq_socket( context, ZMQ_PULL );
        zmq_bind( router, "tcp://*:5559" );
        zmq_bind( pull, "tcp://*:5560" );
        zmq_bind( push, "tcp://*:5561" );

        zmq_pollitem_t items [] = {
                { router, 0, ZMQ_POLLIN, 0 },
                { pull, 0, ZMQ_POLLIN, 0 }
        };

        int more;
        while( 1 ) {
                zmq_msg_t message;
                zmq_poll( items, 2, -1 );
                if( items [0].revents & ZMQ_POLLIN ) {
                        while( 1 ) {
                                zmq_msg_init( &message );
                                zmq_msg_recv( &message, router, 0 );
                                more = zmq_msg_more( &message );
                                zmq_msg_send( &message
                                            , push
                                            , more ? ZMQ_SNDMORE : 0 );
                                if( !more )
                                        break;
                        }
                }
                if( items [1].revents & ZMQ_POLLIN ) {
                        while( 1 ) {
                                zmq_msg_init( &message );
                                zmq_msg_recv( &message, pull, 0 );
                                more = zmq_msg_more( &message );
                                zmq_msg_send( &message
                                            , router
                                            , more ? ZMQ_SNDMORE : 0 );
                                if( !more )
                                        break;
                        }
                }
        }

        zmq_close( router );
        zmq_close( pull );
        zmq_ctx_destroy( context );
        return 0;
}
