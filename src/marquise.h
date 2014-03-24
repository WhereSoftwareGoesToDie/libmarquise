#include <stdint.h>
#include <stdlib.h>

typedef void *marquise_consumer;
typedef void *marquise_connection;

// This can be overriden by the LIBMARQUISE_COLLATOR_MAX_MESSAGES
// environment variable.
#define COLLATOR_MAX_MESSAGES 4096

// This can be overridden by LIBMARQUISE_DEFERRAL_DIR.
#define DEFAULT_DEFERRAL_DIR "/var/tmp"

#define COLLATOR_MAX_RX 131072	// 128 KB

// This can't be more than 65535 without changing the msg_id data type of the
// message_in_flight struct.
#define POLLER_HIGH_WATER_MARK 256

// Microseconds till a message expires
// This can be overridden by the LIBMARQUISE_POLLER_EXPIRY environment
// variable.
#define POLLER_EXPIRY 600000000000	// 10 minutes till we try resending

// How often to check disk for a deferred message
#define POLLER_DEFER_PERIOD 1000000000	// 1 second

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
//
// marquise_consumer objects are not thread-safe.
// FIXME: change that.
marquise_consumer marquise_consumer_new(char *broker, double batch_period);

// Cleanup a consumer's resources. Ensure that you run marquise_close on any
// open connections first.
void marquise_consumer_shutdown(marquise_consumer consumer);

// Open a connection to a consumer. This conection is not thread safe, the
// consumer is. If you wish to send a message from multiple threads, open a new
// connection in each.
marquise_connection marquise_connect(marquise_consumer consumer);
void marquise_close(marquise_connection connection);

// Returns 0 on success, -1 on failure, setting errno. Will only possibly fail
// on zmq_send_msg. This will probably only ever fail if you provide an invalid
// connection.

// Type: TEXT - all text is treated as UTF-8. 
int marquise_send_text(marquise_connection connection, char **source_fields,
		       char **source_values, size_t source_count, char *data,
		       size_t length
		       // nanoseconds
		       , uint64_t timestamp);

// Type: NUMBER
int marquise_send_int(marquise_connection connection, char **source_fields,
		      char **source_values, size_t source_count, int64_t data
		      // nanoseconds
		      , uint64_t timestamp);

// Type: REAL
int marquise_send_real(marquise_connection connection, char **source_fields,
		       char **source_values, size_t source_count, double data
		       // nanoseconds
		       , uint64_t timestamp);

// Type: EMPTY
int marquise_send_counter(marquise_connection connection, char **source_fields,
			  char **source_values, size_t source_count
			  // nanoseconds
			  , uint64_t timestamp);

// Type: BINARY
int marquise_send_binary(marquise_connection connection, char **source_fields,
			 char **source_values, size_t source_count,
			 uint8_t * data, size_t length
			 // nanoseconds
			 , uint64_t timestamp);
