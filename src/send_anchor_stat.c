#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include "anchor_stats.h"

int main () {
        as_consumer consumer     = as_consumer_new("broker.site", 1.5 );
        as_connection connection = as_connect(consumer);

        int i;
	char field_buf[1][4];
	char value_buf[1][4];
	strcpy(field_buf[0], "foo");
	strcpy(value_buf[0], "bar");
        for(i = 0; i < 10000; i++) {
		struct timespec ts;
		uint64_t timestamp;
		int64_t value = 4;
		clock_gettime(CLOCK_REALTIME, &ts);
		timestamp = ts.tv_sec * 1000000000 + ts.tv_nsec;
                as_send_int( connection, field_buf, value_buf, 1, value, timestamp);
        }
        as_close( connection );
        as_consumer_shutdown( consumer );
        return 0;
}
