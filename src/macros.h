#include <syslog.h>
#include <stdlib.h>

#define fail_if( assertion, action, ... )                             \
        if ( assertion ){                                             \
                syslog( LOG_ERR, "libmarquise error: " __VA_ARGS__ ); \
                { action };                                           \
        }

#define free_databurst( b ) \
        free( b->data );    \
        free( b );
