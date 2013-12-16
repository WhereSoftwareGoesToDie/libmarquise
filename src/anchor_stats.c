#include "anchor_stats.h"
#include "zmq_helpers.c"
#include "../config.h"

#include <unistd.h>

#include <stdlib.h>
#include <pthread.h>
#include <syslog.h>
#include <zmq.h>
#include <stdio.h>
#include <glib.h>

#define fail_if( assertion, action, ... ) do {                           \
        if ( assertion ){                                                \
                syslog( LOG_ERR, "libanchor_stats error:" __VA_ARGS__ ); \
                { action };                                              \
        } } while( 0 )

typedef struct {
        void *context;
        void *connection;
        void *broker;
        double poll_period;
} queue_args;

static void *queue_loop ( void *queue_socket );

static void send_upstream( GSList *messages
                          , void *user_context
                          , char *broker );

#define ctx_fail_if( assertion, ... ) do {                                     \
        fail_if( assertion                                                     \
               , zmq_close(connection); zmq_ctx_destroy(context); return NULL; \
               , "as_consumer_new: " __VA_ARGS__ );                            \
        } while( 0 )

as_consumer as_consumer_new( char *broker, double poll_period ) {
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
        queue_args *args  = malloc( sizeof( queue_args ) );
        args->context     = context;
        args->connection  = connection;
        args->poll_period = poll_period;

        size_t broker_size = strlen( broker + 1);
        args->broker = malloc( broker_size );
        memcpy( args->broker, broker, broker_size );


        pthread_t queue_pthread;
        int err = pthread_create( &queue_pthread
                                , NULL
                                , queue_loop
                                , args );
        ctx_fail_if( err, "pthread_create returned: '%d'", err );

        err = pthread_detach( queue_pthread );
        ctx_fail_if( err, "pthread_detach returned: '%d'", err );

        // Ready to go as far as we're concerned, return the context.
        return context;
}

// Listen for events, queue them up for the specified time and then send them
// to the specified broker.
static void *queue_loop( void *args_ptr ) {
        queue_args *args = args_ptr;
        GSList *queue = NULL;
        GTimer *timer = g_timer_new();
        gulong ms; // Useless, microseconds for gtimer_elapsed

        // We poll so that we can ensure that we flush at most
        // 100ms late, as opposed to waiting for an event to
        // trigger the next flush.
        zmq_pollitem_t items [] = {
                { args->connection
                , 0
                , ZMQ_POLLIN
                , 0 }
        };

        while( 1 ) {
                if( zmq_poll (items, 1, 100) == -1 )
                {
                        // Oh god what? We should only ever get an ETERM.
                        if( errno != ETERM )
                                syslog( LOG_ERR
                                      , "libanchor_stats error: zmq_poll "
                                                "got unknown error code: %d"
                                      , errno );
                        // We're done. If the user remembered to shutdown all
                        // the sockets, then the consumer, then there can't be
                        // any more messages waiting. Either way, we can't get
                        // them now.
                        break;
                }

                if (items [0].revents & ZMQ_POLLIN) {
                        zmq_msg_t message;
                        zmq_msg_init( &message );

                        fail_if( zmq_msg_recv( &message
                                             , args->connection
                                             , 0 ) == -1
                               ,
                               , "queue_loop: zmq_msg_recv: '%s'"
                               , strerror( errno ) );

                        queue = g_slist_append( queue, &message );
                        // Defer closing for send_upstream
                }

                if( g_timer_elapsed( timer, &ms ) > args->poll_period ) {
                        g_timer_reset(timer);

                        // Our queue is ready to flush, send_upstream will
                        // handle cleanup of the list and the messages
                        // themselves.
                        send_upstream( queue, args->context, args->broker );
                        queue = NULL;
                }

        }

        zmq_close( args->connection );
        free(args);
}

static void send_upstream( GSList *messages
                          , void *user_context
                          , char *broker ) {
        // Yes, there's a lot of state here, what do you expect when you're
        // trying to keep track of deferred messages.
        static void   *upstream;
        static void   *context;
        static void   *dummy_socket;
        static GSList *deferred;

        // We want our own, separate context so that we can send the last of
        // the messages out when the user shuts us down.
        if( !dummy_socket ) dummy_socket = zmq_socket( user_context, ZMQ_PUSH );

        // We block the user's call on as_consumer_shutdown by holding open a
        // dummy socket whenever there are deferred messages.
        if( !context ) context = zmq_ctx_new();

        upstream = zmq_socket( context, ZMQ_PUSH );
        fail_if( !upstream
               ,
               , "send_upstream: zmq_socket: '%s'"
               , strerror( errno ) );
        fail_if( zmq_connect( upstream, broker )
               ,
               , "send_upstream: zmq_connect: '%s'"
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
