#include <syslog.h>
#define fail_if( assertion, action, ... ) do {                           \
        if ( assertion ){                                                \
                syslog( LOG_ERR, "libanchor_stats error:" __VA_ARGS__ ); \
                { action };                                              \
        } } while( 0 )


