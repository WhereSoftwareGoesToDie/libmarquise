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
#include "lz4/lz4hc.h"
#include <string.h>
#include <math.h>

static void debug_log( char *format, ...) {
        if( !getenv( "LIBMARQUISE_DEBUG" ) )
                return;

        va_list args;
        va_start( args, format );
        vfprintf( stderr, format, args );
        va_end( args );
}

static void *connect_upstream_socket( void *context, char *broker ) {
        debug_log( "Initializing upstream socket to %s\n", broker );
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
        int timeout = 10000;
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

static frame *accumulate_databursts( gpointer zmq_message, gint *queue_length )
{
        static frame *accumulator;
        static size_t offset = 0;

        // Reset and return result
        if( !zmq_message && !queue_length ) {
                debug_log( "Resetting accumulator\n" );

                frame *ret = accumulator;
                accumulator = NULL;
                offset = 0;
                return ret;
        }

        if( *queue_length <= 0 ) {
                return NULL;
        }

        if( !accumulator ) {
                debug_log( "Initializing accumulator for queue length %u\n"
                         , *queue_length );
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
static void try_send_upstream( data_burst *burst, queue_args *args ) {
        debug_log( "Attempting to send burst of len %d upstream\n"
                 , burst->length );

        int ret;

        retry_send:
        // This will timeout due to ZMQ_SNDTIMEO being set on the socket.
        ret = zmq_send( args->upstream_connection
                      , burst->data
                      , burst->length
                      , 0 );
        // I belive that this is reduntant due to ZMQ_SNDTIMEO. It is more me
        // being paranoid.
        if( ret == -1 && errno == EINTR )
                goto retry_send;
        debug_log( "Upstream zmq_send() returned %d\n", ret );

        fail_if( ret == -1
               , marquise_defer_to_file( args->deferral_file, burst );
                 free_databurst( burst );
                 return;
               , "sending message: '%s'", strerror( errno ) );

        // This will timeout also due to ZMQ_RCVTIMEO
        zmq_msg_t response;
        zmq_msg_init( &response );

        retry_recv:
        ret = zmq_msg_recv( &response, args->upstream_connection, 0 );
        if( ret == -1 && errno == EINTR )
                goto retry_recv;
        debug_log( "Upstream zmq_recv() returned %d\n", ret );

        // If a recv fails, we need to reset the connection or the state
        // machine will be out of step.
        fail_if( ret == -1
               , marquise_defer_to_file( args->deferral_file, burst );
                 free_databurst( burst );
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

        // The second failure case is that upstream sends back an error as the
        // ack
        fail_if( ret > 0
               , marquise_defer_to_file( args->deferral_file, burst );
                 free_databurst( burst );
                 return;
               , "recieved upstream error: '%.*s'"
               , ret
               , (char *) zmq_msg_data( &response ) );

        // This may in future contain something useful, like a hash of the
        // message that we can verify.
        zmq_msg_close( &response );
        free_databurst( burst );
}

// Bit twiddling is to support big endian architectures only. Really we are
// just writing two little endian uint32_t's which represent the original
// length, and the compressed length. The compressed data follows.
void add_header( uint8_t *dest, uint32_t original_len, uint32_t data_len ) {
        dest[3]= (original_len >> 24) & 0xff;
        dest[2]= (original_len >> 16) & 0xff;
        dest[1]= (original_len >> 8)  & 0xff;
        dest[0]=  original_len        & 0xff;

        dest[7]= (data_len >> 24) & 0xff;
        dest[6]= (data_len >> 16) & 0xff;
        dest[5]= (data_len >> 8)  & 0xff;
        dest[4]=  data_len        & 0xff;
}

// Returns -1 on failure, 0 on success
static int compress_burst( data_burst *burst ) {
        uint8_t *uncompressed = burst->data;
        // + 8 for the serialized data.
        burst->data = malloc( LZ4_compressBound( burst->length ) + 8 );
        if( !burst->data ) return -1;
        int written = LZ4_compressHC( (char *)uncompressed
                                    , (char *)burst->data + 8
                                    , (int)burst->length );
        if( !written ) return -1;
        add_header( burst->data, burst->length, written );
        burst->length = (size_t)written + 8;
        free( uncompressed );
        return 0;
}

static void send_queue( queue_args *args ) {
        debug_log("Sending queue\n");
        pthread_mutex_lock( &args->queue_mutex );
        if( !args->queue ) {
                pthread_mutex_unlock( &args->queue_mutex );
                return;
        }

        // This iterates over the entire list, passing each
        // element to accumulate_databursts
        guint queue_length = g_slist_length( args->queue );
        g_slist_foreach( g_slist_reverse( args->queue )
                        , (GFunc)accumulate_databursts
                        , &queue_length );
        g_slist_free( args->queue );
        args->queue = NULL;
        pthread_mutex_unlock( &args->queue_mutex );

        frame *frames = accumulate_databursts( NULL, NULL );
        data_burst *burst = malloc( sizeof( data_burst ) );
        size_t max_burst_length =
                get_databurst_size( frames, queue_length );
        burst->data = malloc( max_burst_length );
        burst->length = aggregate_frames( frames
                                        , queue_length
                                        , burst->data );
        debug_log( "Accumulated bursts serialized to %d bytes\n"
                 , burst->length);
        free( frames );
        fail_if( compress_burst( burst )
               , return;
               , "lz4 compression failed" );

        debug_log( "Accumulated bursts compressed to %d bytes\n"
                 , burst->length);

        try_send_upstream( burst, args );
}

static void *flush_queue( void *args_ptr ) {
        queue_args *args = args_ptr;
        pthread_mutex_lock( &args->flush_mutex );

        // And from the dead, a wild databurst appears!
        data_burst *zombie =
                marquise_retrieve_from_file( args->deferral_file );
        if( zombie )
                try_send_upstream( zombie, args );

        send_queue( args );
        pthread_mutex_unlock( &args->flush_mutex );
        return NULL;
}

static void *queue_loop( void *args_ptr ) {
        queue_args *args = args_ptr;
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
                // fail_if
                if( zmq_poll( items, 1, poll_ms ) == -1 ) {
                                syslog( LOG_ERR
                                , "libmarquise error: zmq_poll "
                                        "got unknown error code: %d"
                                , errno );
                                break;
                }

                if ( items [0].revents & ZMQ_POLLIN ) {
                        zmq_msg_t *message = malloc( sizeof( zmq_msg_t ) );
                        if( !message ) continue;
                        zmq_msg_init( message );
                        int rcv = zmq_msg_recv( message
                                        , args->queue_connection
                                        , 0 );

                        fail_if( rcv == -1 
                               ,
                               , "queue_loop: zmq_msg_recv: '%s'"
                               , strerror( errno ) );

                        if(  rcv == 3
                          && !strncmp( zmq_msg_data( message ), "DIE", 3 ) )
                                break;

                        // Prepend here so as to not traverse the entire list.
                        // We will reverse the list when we send it.
                        pthread_mutex_lock( &args->queue_mutex );
                        args->queue = g_slist_prepend( args->queue, message );
                        pthread_mutex_unlock( &args->queue_mutex );

                        // Notify that we have processed the message.
                        fail_if( zmq_send( args->queue_connection, "", 0, 0 )
                        ,
                        , "queue_loop: zmq_send: '%s'"
                        , strerror( errno ) );
                }

                // Fire off a flushing thread every poll period
                if( g_timer_elapsed( timer, &ms ) > args->poll_period ) {
                        debug_log( "Timer has triggered, flushing current queue\n" );
                        g_timer_reset(timer);

                        // If there is already a flushing lock, we need not
                        // start a new thread. This way we can avoid piling up
                        // threads whilst we are failing to recive acks and the
                        // poll period will effectively lower to the ack
                        // timeout.
                        if( pthread_mutex_trylock( &args->queue_mutex ) )
                                continue;

                        // We got the lock, release it so that the flush_thread
                        // may acquire it.
                        pthread_mutex_unlock( &args->queue_mutex );

                        pthread_t flush_thread;
                        int err = pthread_create( &flush_thread
                                                , NULL
                                                , flush_queue
                                                , args );

                        fail_if( err
                               , continue;
                               , "pthread_create returned: '%d'", err );

                        err = pthread_detach( flush_thread );
                        fail_if( err
                               ,
                               , "pthread_detach returned: '%d'", err );

                }
        }

        // Flush the queue one last time before exit
        pthread_mutex_lock( &args->flush_mutex );
        send_queue( args );


        // Ensure that any messages deferred to disk are sent.
        data_burst *remaining;
        while( (remaining = marquise_retrieve_from_file( args->deferral_file)) ) {
                usleep( poll_ms * 1000 );
                try_send_upstream( remaining, args );
        }

        // All messages are sent, including those deferred to disk. It's safe
        // to destroy the context and close the socket.  Will block here untill
        // the last message is on the wire.
        zmq_close( args->upstream_connection );
        zmq_ctx_destroy( args->upstream_context );

        marquise_deferral_file_close( args->deferral_file );
        marquise_deferral_file_free( args->deferral_file );

        // All done, we can send an ack to let marquise_consumer_shutdown
        // return
        zmq_send( args->queue_connection, "", 0, 0 );
        free(args);
        zmq_close( args->queue_connection );
        pthread_mutex_unlock( &args->flush_mutex );
        return NULL;
}

#define ctx_fail_if( assertion, action, ... )         \
        fail_if( assertion                            \
               , { action };                          \
                 zmq_ctx_destroy( context );          \
                 zmq_ctx_destroy( upstream_context ); \
                 return NULL;                         \
               , "marquise_consumer_new: " __VA_ARGS__ );   \

marquise_consumer marquise_consumer_new( char *broker, double poll_period ) {
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
        // Set up the queuing REP socket.
        void *queue_connection = zmq_socket( context, ZMQ_REP );
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

        ctx_fail_if( pthread_mutex_init( &args->queue_mutex, NULL)
                   , zmq_close( queue_connection );
                     zmq_close( upstream_connection );
                   , "pthread_mutex_init" );

        ctx_fail_if( pthread_mutex_init( &args->flush_mutex, NULL)
                   , zmq_close( queue_connection );
                     zmq_close( upstream_connection );
                   , "pthread_mutex_init" );

        args->context             = context;
        args->queue               = NULL;
        args->queue_connection    = queue_connection;
        args->upstream_context    = upstream_context;
        args->upstream_connection = upstream_connection;
        args->poll_period         = poll_period;
        args->deferral_file       = marquise_deferral_file_new();
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

void marquise_consumer_shutdown( marquise_consumer consumer ) {
        void *connection = marquise_connect( consumer );
        if( !connection )
                return;

        if( zmq_send( connection, "DIE", 3, 0 ) == -1 )
                return;


        // The consumer thread will signal when it is done cleaning up.
        zmq_msg_t ack;
        zmq_msg_init( &ack );

        int err;
        retry:
        err = zmq_recvmsg( connection, &ack, 0 );
        if( err == -1 && errno == EINTR ) goto retry;
        fail_if( err == -1
               , return;
               , "zmq_recvmsg: %s", strerror( errno ) );
        zmq_msg_close( &ack );

        zmq_close( connection );
        zmq_ctx_destroy( consumer );
}

#define conn_fail_if( assertion, ... ) do {            \
        fail_if( assertion                             \
               , zmq_close( connection ); return NULL; \
               , __VA_ARGS__ );                        \
        } while( 0 )

marquise_connection marquise_connect( marquise_consumer consumer ) {
        void *connection = zmq_socket( consumer, ZMQ_REQ );
        conn_fail_if( !connection
                    , "marquise_connect: zmq_socket: '%s'"
                    , strerror( errno ) );
        conn_fail_if( zmq_connect( connection, "inproc://queue" )
                    , "marquise_connect: zmq_connect: '%s'"
                    , strerror( errno ) );
        return connection;
}

void marquise_close( marquise_connection connection ) {
        zmq_close( connection );
}

int marquise_send_frame( marquise_connection connection
                 , DataFrame *frame) {
        size_t length = data_frame__get_packed_size( frame );
        uint8_t *marshalled_frame = malloc( length );
        if( !marshalled_frame ) return -1;

        data_frame__pack( frame, marshalled_frame );
        free_frame( frame );

        int ret;
        retry_send:
        ret = zmq_send( connection, marshalled_frame, length, 0);
        if( ret == -1  && errno == EINTR )
                goto retry_send;

        zmq_msg_t ack;
        zmq_msg_init( &ack );
        retry_recv:
        ret = zmq_msg_recv( &ack, connection, 0 );
        if( ret == -1  && errno == EINTR )
                goto retry_recv;

        zmq_msg_close( &ack );

        free( marshalled_frame );
        return ret;
}

int marquise_send_text( marquise_connection connection
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
        return marquise_send_frame( connection, frame );
}

int marquise_send_int( marquise_connection connection
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
        return marquise_send_frame( connection, frame );
}

int marquise_send_real( marquise_connection connection
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
        return marquise_send_frame( connection, frame );
}

int marquise_send_counter( marquise_connection connection
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
        return marquise_send_frame( connection, frame );
}

int marquise_send_binary( marquise_connection connection
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
        return marquise_send_frame( connection, frame );
}
