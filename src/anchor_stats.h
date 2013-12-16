typedef void *as_consumer;
typedef void *as_connection;

// Attempt to start a consumer with the system's local configuration. Returns
// NULL on failure and sets errno.
//
// broker is to be specified as a zmq URI
//
// poll_period is the number of seconds, e.g. 1.3
//
// Failure almost certainly means catastrophic failure, do not retry on
// failure, check syslog.
as_consumer as_consumer_new( char *broker, double poll_period );

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
           , char **tag_fields
           , char **tag_values
           , int tag_count
           , char *data
           // nanoseconds
           , unsigned long long timestamp);

// Type: NUMBER
int as_send_int( as_connection connection
           , char **tag_fields
           , char **tag_values
           , int tag_count
           , int data
           // nanoseconds
           , unsigned long long timestamp);

// Type: REAL
int as_send_real( as_connection connection
           , char **tag_fields
           , char **tag_values
           , int tag_count
           , double data
           // nanoseconds
           , unsigned long long timestamp);

// Type: EMPTY
int as_send_counter( as_connection connection
           , char **tag_fields
           , char **tag_values
           , int tag_count
           // nanoseconds
           , unsigned long long timestamp);

// Type: BINARY
int as_send_binary( as_connection connection
           , char **tag_fields
           , char **tag_values
           , int tag_count
           , char *data
           // nanoseconds
           , unsigned long long timestamp);
