#include <zmq.h>
#include <assert.h>

static inline void switch_msg_from_to( void *from, void *to ) {
        zmq_msg_t msg;
        int more = 1;

        while( more ) {
                zmq_msg_init( &msg );
                int ret = zmq_msg_recv( &msg, from, 0 );
                assert( ret != -1 );
                more = zmq_msg_more( &msg );
                ret = zmq_msg_send( &msg, to, more ? ZMQ_SNDMORE : 0 );
                assert( ret != -1 );
        }
}

int main( void )
{
        void *context = zmq_ctx_new();
        assert( context );
        void *router = zmq_socket( context, ZMQ_ROUTER );
        assert( router );
        void *push = zmq_socket( context, ZMQ_PUSH );
        assert( push );
        void *pull = zmq_socket( context, ZMQ_PULL );
        assert( pull );

        int err = zmq_bind( router, "tcp://*:5559" );
        assert( !err );
        err = zmq_bind( push, "tcp://*:5561" );
        assert( !err );
        err = zmq_bind( pull, "tcp://*:5560" );
        assert( !err );

        zmq_pollitem_t items [] = {
                { router, 0, ZMQ_POLLIN, 0 },
                { pull, 0, ZMQ_POLLIN, 0 }
        };

        while( 1 ) {
                err = zmq_poll( items, 2, -1 );
                assert( err != -1 );

                if( items [0].revents & ZMQ_POLLIN )
                        switch_msg_from_to( router, push );

                if( items [1].revents & ZMQ_POLLIN )
                        switch_msg_from_to( pull, router );
        }
}
