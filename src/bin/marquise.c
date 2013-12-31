#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include "marquise.h"
#include "assert.h"

int main( int argc, char **argv ) {
        if( argc != 4 ) {
                puts( "usage: marquise test [broker] [iterations]" );
                exit( 1 );
        }

        if( strcasecmp( argv[1], "test" ) ) {
                printf( "invalid command: %s", argv[1] );
                exit( 1 );
        }

        marquise_consumer consumer     = marquise_consumer_new( argv[2], 1 );
        if( !consumer ) {
                perror( "marquise_consumer_new" );
                exit( 1 );
        }

        marquise_connection connection = marquise_connect(consumer);
        if( !consumer ) {
                perror( "marquise_connect" );
                exit( 1 );
        }

        int n = atoi( argv[3] );
        if( n < 0 ) n = 0;


        int i;
        char *field_buf[] = {"foo"};
        char *value_buf[] = {"bar"};
        for(i = 1; i <= n; i++) {
                struct timespec ts;
                uint64_t timestamp;
                int64_t value = i;
                clock_gettime(CLOCK_REALTIME, &ts);
                timestamp = ts.tv_sec * 1000000000 + ts.tv_nsec;
                int ret = marquise_send_int( connection, field_buf, value_buf, 1, value, timestamp);
                assert( ret != -1 );
        }

        printf( "Sent %d frames, waiting for ack.\n", n );

        marquise_close( connection );
        marquise_consumer_shutdown( consumer );
        puts( "Got ack, shutting down." );
        return 0;
}
