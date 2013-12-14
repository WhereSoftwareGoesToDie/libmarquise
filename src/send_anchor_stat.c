#include <stdio.h>
#include <anchor_stats.h>

int main () {
        as_consumer consumer     = as_consumer_new("broker.site", 1 );
        as_connection connection = as_connect(consumer);

        int i;
        for(i = 0; i < 1000000; i++) {
                as_send( connection, "key", "data", 24, 0 );
                puts("done sending");
        }
        as_close( connection );
        as_consumer_shutdown( consumer );
        return 0;
}
