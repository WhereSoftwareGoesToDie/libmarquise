#include "anchor_stats.h"
#include "zmq_helpers.c"
#include "../config.h"

#include <unistd.h>

#include <stdlib.h>
#include <pthread.h>
#include <syslog.h>
#include <zmq.h>
#include <stdio.h>

#define fail_if( assertion, action, ... ) do {                           \
        if ( assertion ){                                                \
                syslog( LOG_ERR, "libanchor_stats error:" __VA_ARGS__ ); \
                { action };                                              \
        } } while( 0 )

typedef struct {
        void *context;
        void *connection;
        void *broker;
} queue_args;

static void *start_queue ( void *queue_socket );

#define ctx_fail_if( assertion, ... ) do {                                     \
        fail_if( assertion                                                     \
               , zmq_close(connection); zmq_ctx_destroy(context); return NULL; \
               , "as_consumer_new: " __VA_ARGS__ );                            \
        } while( 0 )

as_consumer as_consumer_new( char *broker, unsigned int poll_period ) {
        // This context is used for fast inprocess communication, we pass it to
        // the queuing thread which will connect back to us and start
        // processing requests.
        void *context = zmq_ctx_new();
        fail_if( !context, , "zmq_ctx_new failed, this is very confusing." );

        // Set up the queuing PULL socket.
        void *connection = zmq_socket( context, ZMQ_PULL );
        ctx_fail_if( !connection, "zmq_socket: '%s'", strerror( errno ) );

        ctx_fail_if( zmq_bind( connection, "inproc://queue" )
                   , "zmq_bind: '%s'"
                   , strerror( errno ) );

        // Our new thread gets it's own copy of the zmq context, a brand new
        // shiny PULL inproc socket, and the broker that the user would like to
        // connect to.
        queue_args *args = malloc( sizeof( queue_args ) );
        args->context   = context;
        args->connection = connection;

        size_t broker_size = strlen( broker + 1);
        args->broker = malloc( broker_size );
        memcpy( args->broker, broker, broker_size );


        pthread_t queue_pthread;
        int err = pthread_create( &queue_pthread
                                , NULL
                                , start_queue
                                , args );
        ctx_fail_if( err, "pthread_create returned: '%d'", err );

        err = pthread_detach( queue_pthread );
        ctx_fail_if( err, "pthread_detach returned: '%d'", err );

        // Ready to go as far as we're concerned, return the context.
        return context;
}

// Listen for events, queue them up for the specified time and then send them
// to the specified broker.
static void *start_queue( void *args_ptr ) {
        queue_args *args = args_ptr;

        while( 1 ) {
                zmq_msg_t message;
                zmq_msg_init( &message );

                int err = zmq_msg_recv( &message, args->connection, 0 );
                // Send upstream
        }
}

static void *send_upstream( zmq_msg_t messages[]
                          , size_t n_messages
                          , void *context
                          , char *broker ) {
        static void *upstream;

        upstream = zmq_socket( context, ZMQ_PUSH );
        fail_if( !upstream
               ,
               , "start_queue: zmq_socket: '%s'"
               , strerror( errno ) );
        fail_if( zmq_connect( upstream, broker )
               ,
               , "start_queue: zmq_connect: '%s'"
               , strerror( errno ) );
};

void as_consumer_shutdown( as_consumer consumer ) {
        zmq_ctx_destroy( consumer );
}

#define conn_fail_if( assertion, ... ) do {            \
        fail_if( assertion                             \
               , zmq_close( connection ); return NULL; \
               , __VA_ARGS__ );                        \
        } while( 0 )

as_connection as_connect( as_consumer consumer ) {
        void *connection = zmq_socket( consumer, ZMQ_PUSH );
        conn_fail_if( !connection
                    , "as_connect: zmq_socket: '%s'"
                    , strerror( errno ) );
        conn_fail_if( zmq_connect( connection, "inproc://queue" )
                    , "as_connect: zmq_connect: '%s'"
                    , strerror( errno ) );
        return connection;
}

void as_close( as_connection connection ) {
        zmq_close( connection );
}

int as_send( as_connection connection
              , char *key
              , char *data
              , unsigned int seconds
              , unsigned int milliseconds ){
        // TODO: Serialize, compress then encrypt.
        fail_if( s_send( connection, data ) == -1
               , return -1;
               , "as_send: s_send: '%s'"
               , strerror( errno ) );
        return 0;
}
