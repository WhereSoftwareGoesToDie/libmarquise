#include <syslog.h>
#include <stdlib.h>

#define fail_if( assertion, action, ... ) do {                            \
        if ( assertion ){                                                 \
                syslog( LOG_ERR, "libmarquise error: " __VA_ARGS__ ); \
                { action };                                               \
        } } while( 0 )

#define free_databurst( b ) do { \
        free( b->data );         \
        free( b );               \
} while( 0 )
