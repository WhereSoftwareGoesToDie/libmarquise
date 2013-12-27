#include "marquise.h"
#include "structs.h"
#include "frame.h"
#include "macros.h"
#include "../config.h"
#include "defer.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <zmq.h>
#include <stdio.h>
#include <glib.h>
#include <sys/mman.h>
#include "lz4/lz4.h"
#include <string.h>
#include <math.h>

static void *queue_loop ( void *queue_socket );

static void *connect_upstream_socket( void *context, char *broker ) {
        // Set up the upstream REQ socket.
        void *upstream_connection = zmq_socket( context, ZMQ_REQ );

        fail_if( zmq_connect( upstream_connection, broker )
                   , zmq_close( upstream_connection ); return NULL;
                   , "zmq_connect: '%s'"
                   , strerror( errno ) );

        // We want to be explicit about the internal high water mark:
        // After this many poll periods have elapsed, zmq_sendmsg will start
        // returning EAGAIN and we will start deferring further messages to
        // disk to save memory.
        int one = 1;

        fail_if( zmq_setsockopt( upstream_connection, ZMQ_SNDHWM, &one, sizeof( one ) )
                   , zmq_close( upstream_connection );
                   , "zmq_setsockopt ZMQ_HWM: '%s'"
                   , strerror( errno ) );

        // Milliseconds
        int timeout = 1000;
        fail_if( zmq_setsockopt( upstream_connection
                                   , ZMQ_SNDTIMEO
                                   , &timeout
                                   , sizeof( timeout ) )
                   , zmq_close( upstream_connection ); return NULL;
                   , "zmq_setsockopt ZMQ_SNDTIMEO: '%s'"
                   , strerror( errno ) );

        // Milliseconds
        fail_if( zmq_setsockopt( upstream_connection
                                   , ZMQ_RCVTIMEO
                                   , &timeout
                                   , sizeof( timeout ) )
                   , zmq_close( upstream_connection ); return NULL;
                   , "zmq_setsockopt ZMQ_SNDTIMEO: '%s'"
                   , strerror( errno ) );

        return upstream_connection;
}

#define ctx_fail_if( assertion, action, ... )         \
        fail_if( assertion                            \
               , { action };                          \
                 zmq_ctx_destroy( context );          \
                 zmq_ctx_destroy( upstream_context ); \
                 return NULL;                         \
               , "as_consumer_new: " __VA_ARGS__ );   \

as_consumer as_consumer_new( char *broker, double poll_period ) {
        // This context is used for fast inprocess communication, we pass it to
        // the queuing thread which will connect back to us and start
        // processing requests.
        void *context = zmq_ctx_new();
        fail_if( !context
               , return NULL;
               , "zmq_ctx_new failed, this is very confusing." );

        void *upstream_context = zmq_ctx_new();
        fail_if( !upstream_context
               , zmq_ctx_destroy( context ); return NULL;
               , "zmq_ctx_new (upstream) failed, this is very confusing." );

        ctx_fail_if( poll_period <= 0, , "poll_period cannot be <= 0" );
        // Set up the queuing PULL socket.
        void *queue_connection = zmq_socket( context, ZMQ_PULL );
        ctx_fail_if( !queue_connection
                   , zmq_close( queue_connection );
                   , "zmq_socket: '%s'"
                   , strerror( errno ) );

        ctx_fail_if( zmq_bind( queue_connection, "inproc://queue" )
                   , zmq_close( queue_connection );
                   , "zmq_bind: '%s'"
                   , strerror( errno ) );

        // We set this upstream socket up here only to fail early.
        void *upstream_connection = connect_upstream_socket( upstream_context, broker );
        if( !upstream_connection ) {
                zmq_close( queue_connection );
                zmq_ctx_destroy( context );
                zmq_ctx_destroy( upstream_context );
                return NULL;
        }

        // Our new thread gets it's own copy of the zmq context, a brand new
        // shiny PULL inproc socket, and the broker that the user would like to
        // connect to.
        queue_args *args          = malloc( sizeof( queue_args ) );
        ctx_fail_if( !args
                   , zmq_close( queue_connection );
                     zmq_close( upstream_connection );
                   , "malloc()" );

        args->context             = context;
        args->queue_connection    = queue_connection;
        args->upstream_context    = upstream_context;
        args->upstream_connection = upstream_connection;
        args->poll_period         = poll_period;
        args->deferral_file       = as_deferral_file_new();
        ctx_fail_if( ! args->deferral_file
                   , zmq_close( queue_connection );
                     zmq_close( upstream_connection );
                   , "mkstemp: '%s'"
                   , strerror( errno ) );
        args->broker = malloc( strlen( broker ) + 1 );
        ctx_fail_if( !args
                   , zmq_close( queue_connection );
                     zmq_close( upstream_connection );
                   , "malloc()" );
        strcpy( args->broker, broker );

        pthread_t queue_pthread;
        int err = pthread_create( &queue_pthread
                                , NULL
                                , queue_loop
                                , args );
        ctx_fail_if( err
                   , zmq_close( queue_connection );
                     zmq_close( upstream_connection );
                   , "pthread_create returned: '%d'", err );

        err = pthread_detach( queue_pthread );
        ctx_fail_if( err
                   , zmq_close( queue_connection );
                     zmq_close( upstream_connection );
                   , "pthread_detach returned: '%d'", err );

        // Ready to go as far as we're concerned, return the context.
        return context;
}

static frame *accumulate_databursts( gpointer zmq_message, gint *queue_length )
{
        static frame *accumulator;
        static size_t offset = 0;

        // Reset and return result
        if( !zmq_message && !queue_length ) {
                frame *ret = accumulator;
                accumulator = NULL;
                offset = 0;
                return ret;
        }

        if( *queue_length <= 0 ) {
                return NULL;
        }

        if( !accumulator ) {
                accumulator = calloc( *queue_length, sizeof( frame ) );
                if( !accumulator ) {
                        *queue_length -= 1;
                        return NULL;
                }
        }

        size_t msg_size = zmq_msg_size( zmq_message );
        accumulator[offset].length = msg_size;
        accumulator[offset].data = malloc( msg_size );
        if( !accumulator[offset].data ) {
                *queue_length -= 1;
                return NULL;
        }
        memcpy( accumulator[offset].data, zmq_msg_data( zmq_message ), msg_size );
        zmq_msg_close( zmq_message );
        free( zmq_message );
        offset++;

        return NULL;
}

// This function either sends the message or defers it. There is no try.
static void try_send_upstream(data_burst *burst, queue_args *args ) {
        // This will timeout due to ZMQ_SNDTIMEO being set on the socket.
        fail_if( zmq_send( args->upstream_connection
                         , burst->data
                         , burst->length
                         , 0 ) == -1
               , as_defer_to_file( args->deferral_file, burst ); free_databurst( burst ); return;
               , "sending message: '%s'", strerror( errno ) );

        // This will timeout also due to ZMQ_RCVTIMEO
        zmq_msg_t response;
        zmq_msg_init( &response );
        fail_if( zmq_msg_recv( &response, args->upstream_connection, 0 ) == -1
               , as_defer_to_file( args->deferral_file, burst );
                 free_databurst( burst );
                 // If a recv fails, we need to reset the connection or the
                 // state machine will be out of step.
                 int linger = 0;
                 zmq_setsockopt( args->upstream_connection
                               , ZMQ_LINGER
                               , &linger
                               , sizeof( linger ) );
                 zmq_close( args->upstream_connection );
                 args->upstream_connection = connect_upstream_socket(
                           args->upstream_context
                         , args->broker );
                 return;
               , "receiving ack: '%s'", strerror( errno ) );

        // This may in future contain something useful, like a hash of the
        // message that we can verify.
        zmq_msg_close( &response );
        free_databurst( burst );
}

static void send_queue( GSList **queue, queue_args *args ) {
        // This iterates over the entire list, passing each
        // element to accumulate_databursts
        guint queue_length = g_slist_length( *queue );
        g_slist_foreach( g_slist_reverse( *queue )
                        , (GFunc)accumulate_databursts
                        , &queue_length );
        g_slist_free( *queue );
        *queue = NULL;

        frame *frames = accumulate_databursts( NULL, NULL );
        data_burst *burst = malloc( sizeof( data_burst ) );
        size_t max_burst_length =
                get_databurst_size( frames, queue_length );
        burst->data = malloc( max_burst_length );
        burst->length = aggregate_frames( frames
                                        , queue_length
                                        , burst->data );
        free( frames );

        try_send_upstream( burst, args );
}

// Listen for events, queue them up for the specified time and then send them
// to the specified broker.
static void *queue_loop( void *args_ptr ) {
        queue_args *args = args_ptr;
        GSList *queue = NULL;
        GTimer *timer = g_timer_new();
        gulong ms; // Useless, microseconds for gtimer_elapsed
        int poll_ms = floor( 1000 * args->poll_period );

        // We poll so that we can ensure that we flush at most
        // 100ms late, as opposed to waiting for an event to
        // trigger the next flush.
        zmq_pollitem_t items [] = {
                { args->queue_connection
                , 0
                , ZMQ_POLLIN
                , 0 }
        };

        while( 1 ) {
                if( zmq_poll( items, 1, poll_ms ) == -1 ) {
                        // Oh god what? We should only ever get an ETERM.
                        if( errno != ETERM ) {
                                syslog( LOG_ERR
                                      , "libmarquise error: zmq_poll "
                                                "got unknown error code: %d"
                                      , errno );
                        }
                        // We're done. If the user remembered to shutdown all
                        // the sockets, then the consumer, then there can't be
                        // any more messages waiting. Either way, we can't get
                        // them now.
                        break;
                }

                if ( items [0].revents & ZMQ_POLLIN ) {
                        zmq_msg_t *message = malloc( sizeof( zmq_msg_t ) );
                        if( !message ) continue;
                        zmq_msg_init( message );

                        fail_if( zmq_msg_recv( message
                                             , args->queue_connection
                                             , 0 ) == -1
                               ,
                               , "queue_loop: zmq_msg_recv: '%s'"
                               , strerror( errno ) );

                        // Prepend here so as to not traverse the entire list.
                        queue = g_slist_prepend( queue, message );
                }

                if( g_timer_elapsed( timer, &ms ) > args->poll_period ) {
                        g_timer_reset(timer);
                        // And from the dead, a wild databurst appears!
                        data_burst *zombie =
                                as_retrieve_from_file( args->deferral_file );
                        if( zombie )
                                try_send_upstream( zombie, args );

                        if( queue )
                                send_queue( &queue, args );
               }

        }

        g_timer_destroy( timer );

        // Flush the queue one last time before exit.
        if( queue ) send_queue( &queue, args );


        // Ensure that any messages deferred to disk are sent.
        data_burst *remaining;
        while( (remaining = as_retrieve_from_file( args->deferral_file)) ) {
                usleep( poll_ms * 1000 );
                try_send_upstream( remaining, args );
        }

        // All messages are sent, including those deferred to disk. It's safe
        // to destroy the context and close the socket.  Will block here untill
        // the last message is on the wire.
        zmq_close( args->upstream_connection );
        zmq_ctx_destroy( args->upstream_context );

        as_deferral_file_close( args->deferral_file );
        as_deferral_file_free( args->deferral_file );

        // Closing this connection will allow the user's call to
        // as_consumer_shutdown to return.
        zmq_close( args->queue_connection );
        free(args);
        return NULL;
}

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

int as_send_frame( as_connection connection
                 , DataFrame *frame) {
        size_t length = data_frame__get_packed_size( frame );
        uint8_t *marshalled_frame = malloc( length );
        if( !marshalled_frame ) return -1;

        data_frame__pack( frame, marshalled_frame );
        free_frame( frame );

        int ret;
        retry:
        ret = zmq_send( connection, marshalled_frame, length, 0);
        if( ret == -1  && errno == EINTR )
                goto retry;

        free( marshalled_frame );
        return ret;
}

int as_send_text( as_connection connection
           , char **source_fields
           , char **source_values
           , size_t source_count
           , char *data
           , size_t length
           , uint64_t timestamp) {
        DataFrame *frame = build_frame( source_fields
                                      , source_values
                                      , source_count
                                      , timestamp
                                      , DATA_FRAME__TYPE__TEXT );
        if( !frame ) return -1 ;
        frame->value_textual = data;
        return as_send_frame( connection, frame );
}

int as_send_int( as_connection connection
           , char **source_fields
           , char **source_values
           , size_t source_count
           , int64_t data
           , uint64_t timestamp) {
        DataFrame *frame = build_frame( source_fields
                                      , source_values
                                      , source_count
                                      , timestamp
                                      , DATA_FRAME__TYPE__NUMBER);
        if( !frame ) return -1 ;
        frame->value_numeric = data;
        frame->has_value_numeric = 1;
        return as_send_frame( connection, frame );
}

int as_send_real( as_connection connection
           , char **source_fields
           , char **source_values
           , size_t source_count
           , double data
           , uint64_t timestamp) {
        DataFrame *frame = build_frame( source_fields
                                      , source_values
                                      , source_count
                                      , timestamp
                                      , DATA_FRAME__TYPE__REAL);
        if( !frame ) return -1 ;
        frame->value_measurement = data;
        frame->has_value_measurement = 1;
        return as_send_frame( connection, frame );
}

int as_send_counter( as_connection connection
           , char **source_fields
           , char **source_values
           , size_t source_count
           , uint64_t timestamp) {
        DataFrame *frame = build_frame( source_fields
                                      , source_values
                                      , source_count
                                      , timestamp
                                      , DATA_FRAME__TYPE__EMPTY);
        if( !frame ) return -1 ;
        return as_send_frame( connection, frame );
}

int as_send_binary( as_connection connection
           , char **source_fields
           , char **source_values
           , size_t source_count
           , uint8_t *data
           , size_t length
           , uint64_t timestamp) {
        DataFrame *frame = build_frame( source_fields
                                      , source_values
                                      , source_count
                                      , timestamp
                                      , DATA_FRAME__TYPE__BINARY);
        if( !frame ) return -1 ;
        frame->value_blob.len = length;
        frame->value_blob.data = data;
        frame->has_value_blob = 1;
        return as_send_frame( connection, frame );
}
