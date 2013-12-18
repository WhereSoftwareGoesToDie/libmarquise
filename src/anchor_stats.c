#include "anchor_stats.h"
#include "structs.h"
#include "frame.h"
#include "macros.h"
#include "../config.h"
#include "defer.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <zmq.h>
#include <stdio.h>
#include <glib.h>
#include <sys/mman.h>
#include "lz4/lz4.h"
#include <string.h>

static void *queue_loop ( void *queue_socket );

#define ctx_fail_if( assertion, action, ... )                         \
        fail_if( assertion                                            \
               , { action }; zmq_ctx_destroy( context ); return NULL; \
               , "as_consumer_new: " __VA_ARGS__ );                   \

as_consumer as_consumer_new( char *broker, double poll_period ) {
        // This context is used for fast inprocess communication, we pass it to
        // the queuing thread which will connect back to us and start
        // processing requests.
        void *context = zmq_ctx_new();
        fail_if( !context
               , return NULL;
               , "zmq_ctx_new failed, this is very confusing." );

        ctx_fail_if( poll_period < 0.1, , "poll_period cannot be < 0.1" );
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

        // Set up the upstream PUSH socket.
        void *upstream_connection = zmq_socket( context, ZMQ_PUSH );
        ctx_fail_if( !queue_connection
                   , zmq_close( queue_connection );
                   , "zmq_socket: '%s'"
                   , strerror( errno ) );

        ctx_fail_if( zmq_connect( queue_connection, broker )
                   , zmq_close( queue_connection );
                     zmq_close( upstream_connection );
                   , "zmq_connect: '%s'"
                   , strerror( errno ) );

        // We want to be explicit about the internal high water mark:
        // After this many poll periods have elapsed, zmq_sendmsg will start
        // returning EAGAIN and we will start deferring further messages to
        // disk to save memory.
        int hwm = 60 * 60 * 24;

        ctx_fail_if( zmq_setsockopt( upstream_connection, ZMQ_SNDHWM, &hwm, sizeof( hwm ) )
                   , zmq_close( queue_connection );
                     zmq_close( upstream_connection );
                   , "zmq_setsockopt ZMQ_HWM: '%s'"
                   , strerror( errno ) );

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
        args->upstream_connection = upstream_connection;
        args->poll_period         = poll_period;
        args->deferral_file       = as_deferral_file_new();
        ctx_fail_if( ! args->deferral_file
                   , zmq_close( queue_connection );
                     zmq_close( upstream_connection );
                   , "mkstemp: '%s'"
                   , strerror( errno ) );

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
                { args->queue_connection
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
                                             , args->queue_connection
                                             , 0 ) == -1
                               ,
                               , "queue_loop: zmq_msg_recv: '%s'"
                               , strerror( errno ) );

                        queue = g_slist_append( queue, &message );
                        // Defer closing for send_upstream
                }

                if( g_timer_elapsed( timer, &ms ) > args->poll_period ) {
                        g_timer_reset(timer);
                        // Time to flush!

                        // TODO: Serialize, compress and encrypt queue into a
                        // burst, then free it.
                        data_burst *burst = malloc( sizeof( data_burst ) );
                        burst->data = malloc( 5 );
                        strcpy( burst->data, "hai" );
                        burst->length = 4;

                        // Simulating freeing(). This is really fast.
                        queue = NULL;

                        zmq_msg_t message;
                        zmq_msg_init_size( &message, burst->length );
                        memcpy( zmq_msg_data( &message )
                              , burst->data
                              , burst->length );
                        int err = zmq_msg_send( &message
                                              , args->upstream_connection
                                              , ZMQ_DONTWAIT );
                        if( err == -1 ) {
                                if( errno == EAGAIN ){
                                        as_defer_to_file( args->deferral_file
                                                        , burst );
                                }
                        }
                }

        }

        g_timer_destroy( timer );
        zmq_close( args->queue_connection );
        zmq_close( args->upstream_connection );
        as_deferral_file_close( args->deferral_file );
        as_deferral_file_free( args->deferral_file );
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
              , DataFrame frame) {
	uint8_t *marshalled_frame;
	char *compressed_frame;
	unsigned int len;
	unsigned int compressed_size;
	len = data_frame__get_packed_size(&frame);
	marshalled_frame = malloc(len);
	data_frame__pack(&frame, marshalled_frame);
	
        /*fail_if( s_send( connection, data ) == -1
               , return -1;
               , "as_send: s_send: '%s'"
               , strerror( errno ) );*/
        return 0;
}

int as_send_text( as_connection connection
           , char **tag_fields
           , char **tag_values
           , size_t tag_count
           , char *data
           , size_t len
           , uint64_t timestamp) {
	DataFrame frame = build_frame_skel(tag_fields, tag_values, tag_count, timestamp, DATA_FRAME__TYPE__TEXT);
	frame.value_textual = malloc(sizeof(char)*(strlen(data)+1));
	strcpy(frame.value_textual, data);
	return as_send_frame(connection, frame);
}

int as_send_int( as_connection connection
           , char **tag_fields
           , char **tag_values
           , size_t tag_count
           , int64_t data
           , uint64_t timestamp) {
	DataFrame frame = build_frame_skel(tag_fields, tag_values, tag_count, timestamp, DATA_FRAME__TYPE__NUMBER);
	frame.value_numeric = data;
	frame.has_value_numeric = 1;
	return as_send_frame(connection, frame);
}

int as_send_real( as_connection connection
           , char **tag_fields
           , char **tag_values
           , size_t tag_count
           , double data
           , uint64_t timestamp) {
	DataFrame frame = build_frame_skel(tag_fields, tag_values, tag_count, timestamp, DATA_FRAME__TYPE__REAL);
	frame.value_measurement = data;
	frame.has_value_measurement = 1;
	return as_send_frame(connection, frame);
}

int as_send_counter( as_connection connection
           , char **tag_fields
           , char **tag_values
           , size_t tag_count
           , uint64_t timestamp) {
	DataFrame frame = build_frame_skel(tag_fields, tag_values, tag_count, timestamp, DATA_FRAME__TYPE__EMPTY);
	return as_send_frame(connection, frame);
}

int as_send_binary( as_connection connection
           , char **tag_fields
           , char **tag_values
           , size_t tag_count
           , char *data
	   , size_t len
           , uint64_t timestamp) {
	DataFrame frame = build_frame_skel(tag_fields, tag_values, tag_count, timestamp, DATA_FRAME__TYPE__BINARY);
	frame.value_blob.len = len;
	frame.value_blob.data = malloc(sizeof(char)*len);
	memcpy(frame.value_blob.data, data, len);
	frame.has_value_blob = 1;
	return as_send_frame(connection, frame);
}
