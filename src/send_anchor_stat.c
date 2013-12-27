#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include "marquise.h"
#include "assert.h"

int main () {
        as_consumer consumer     = as_consumer_new("tcp://localhost:5559", 1 );
        assert( consumer );
        as_connection connection = as_connect(consumer);
        assert( connection );

        int i;
        char *field_buf[] = {"foo"};
        char *value_buf[] = {"bar"};
        for(i = 0; i < 10000; i++) {
                struct timespec ts;
                uint64_t timestamp;
                int64_t value = 4;
                clock_gettime(CLOCK_REALTIME, &ts);
                timestamp = ts.tv_sec * 1000000000 + ts.tv_nsec;
                int ret = as_send_int( connection, field_buf, value_buf, 1, value, timestamp);
                assert( ret != -1 );
                //usleep( 1000 );
        }
        as_close( connection );
        as_consumer_shutdown( consumer );
        return 0;
}
