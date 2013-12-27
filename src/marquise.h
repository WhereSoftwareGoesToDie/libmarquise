#include <stdint.h>
#include <stdlib.h>

typedef void *as_consumer;
typedef void *as_connection;

// Attempt to start a consumer with the system's local configuration. Returns
// NULL on failure and sets errno.
//
// broker is to be specified as a zmq URI
//
// batch_period is the interval (in seconds) at which the worker thread
// will poll and empty the queue.
//
// Failure almost certainly means catastrophic failure, do not retry on
// failure, check syslog.
as_consumer as_consumer_new( char *broker, double batch_period );

// Cleanup a consumer's resources. Ensure that you run as_close on any
// open connections first.
void as_consumer_shutdown( as_consumer consumer );

// Open a connection to a consumer. This conection is not thread safe, the
// consumer is. If you wish to send a message from multiple threads, open a new
// connection in each.
as_connection as_connect( as_consumer consumer );
void as_close( as_connection connection );

// Returns 0 on success, -1 on failure, setting errno. Will only possibly fail
// on zmq_send_msg. This will probably only ever fail if you provide an invalid
// connection.

// Type: TEXT
int as_send_text( as_connection connection
                , char **source_fields
                , char **source_values
                , size_t source_count
                , char *data
                , size_t length
                // nanoseconds
                , uint64_t timestamp);

// Type: NUMBER
int as_send_int( as_connection connection
               , char **source_fields
               , char **source_values
               , size_t source_count
               , int64_t data
               // nanoseconds
               , uint64_t timestamp);

// Type: REAL
int as_send_real( as_connection connection
                , char **source_fields
                , char **source_values
                , size_t source_count
                , double data
                // nanoseconds
                , uint64_t timestamp);

// Type: EMPTY
int as_send_counter( as_connection connection
                   , char **source_fields
                   , char **source_values
                   , size_t source_count
                   // nanoseconds
                   , uint64_t timestamp);

// Type: BINARY
int as_send_binary( as_connection connection
                  , char **source_fields
                  , char **source_values
                  , size_t source_count
                  , uint8_t *data
                  , size_t length
                  // nanoseconds
                  , uint64_t timestamp);
