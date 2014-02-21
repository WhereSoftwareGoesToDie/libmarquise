/*
 * marquised
 *
 * Act as a proxy for vaultaire connections so that user processes
 * don't have to stay running until all data is confirmed into the
 * vault.
 *
 * This isn't required for using vaultaire. User processes can talk
 * directly to the broker if they wish.
 *
 * FIXME:
 * 	* Currently does not honour the origin of the user data
 * 	  and substitutes it's own
 * TODO:
 * 	* Handle signals for graceful flush and exit
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <endian.h>
#include <assert.h>
#include <zmq.h>
#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINTF(...) fprintf(stderr, __VA_ARGS__);
#else
#define DEBUG_PRINTF(...)
#endif

#define DEFAULT_LISTEN_ADDRESS	"tcp://127.0.0.1:5560/"

/* most inefficient hexdump on the planet */
static void fhexdump(FILE *fp, uint8_t *buf, size_t bufsiz) {
	while (bufsiz--)
		fprintf(fp,"%02x", *(buf++));
}

int main(int argc, char **argv) {
	char *zmq_listen_address;
	char *zmq_broker_address;
	void *zmq_listen_sock;
	int verbose = 0;
#define verbose_printf(...) { if (verbose) fprintf(stderr, __VA_ARGS__); }


	/* Parse command line
	 */
	if (argc < 2) {
		fprintf(stderr, "%s <upstream broker socket> [ <listen socket address> ]\n\n"
				"\tThe default listen address is " DEFAULT_LISTEN_ADDRESS "\n\n"
				, argv[0]);
		return 1;
	}
	argv++; argc--;
	while (argc > 1) {
		if (strncmp("-v", *argv, 3) == 0)
			verbose = 1;
		else break;
		argv++; argc--;
	}
	zmq_broker_address = *(argv++); argc--;
	if (argc) {
		zmq_listen_address = *(argv++); argc--;
	} else {
		zmq_listen_address = DEFAULT_LISTEN_ADDRESS;
	}
		
	void *zmq_context = zmq_ctx_new();

	/*
	 * Set up libmarquis' poller component to talk to the broker and
	 * deal with retries / disk deferral etc.
	 *
	 * We don't need the collation thread as frames are already grouped
	 * into DataBursts and compressed by the client
	 */

	// FIXME

	/* 
	 * Bind the zmq socket to listen
	 */
	verbose_printf("binding listen socket to %s\n", zmq_listen_address);
	zmq_listen_sock = zmq_socket(zmq_context, ZMQ_ROUTER);
	if (zmq_bind(zmq_listen_sock, zmq_listen_address)) 
		return perror("zmq_bind"), 1;

	/*
	 * Receive handler.
	 *
	 *	* receive the message
	 *	* write out
	 *	* ack once write successful
	 */
	while(1) {
		zmq_msg_t ident, msg_id, burst;
		zmq_msg_init(&ident);
		zmq_msg_init(&msg_id);
		zmq_msg_init(&burst);

		/* Receive the databurst */
		size_t ident_rx;
		size_t msg_id_rx;
		size_t burst_rx;

		errno = 0;
		do { ident_rx  = zmq_msg_recv(&ident, zmq_listen_sock, 0);
		} while (errno == EINTR);
		if (ident_rx < 1 ) return perror("zmq_msg_recv (ident_rx)"), 1;
		if (!zmq_msg_more(&ident)) {
			fprintf(stderr, "Got short message (only 1 part). Skipping");
			zmq_msg_close(&ident);
			continue;
		}

		errno = 0;
		do { msg_id_rx = zmq_msg_recv(&msg_id, zmq_listen_sock, 0);
		} while (errno == EINTR);
		if (msg_id_rx < 1 ) return perror("zmq_msg_recv (msg_id_rx)"), 1;
		if (!zmq_msg_more(&msg_id)) {
			fprintf(stderr, "Got short message (only 2 parts). Skipping");
			zmq_msg_close(&ident); zmq_msg_close(&msg_id);
			continue;
		}

		errno = 0;
		do { burst_rx = zmq_msg_recv(&burst, zmq_listen_sock, 0);
		} while (errno == EINTR);
		if (burst_rx < 0 ) 
			return perror("zmq_msg_recv (burst_rx)"), 1;

		if (verbose) {
			fprintf(stderr, "received %lu bytes\n\tidentity:\t0x", zmq_msg_size(&burst));
			fhexdump(stderr, zmq_msg_data(&ident), zmq_msg_size(&ident));
			fprintf(stderr, "\n\tmessage id:\t0x");
			fhexdump(stderr, zmq_msg_data(&msg_id), zmq_msg_size(&msg_id));
			fputc('\n', stderr);
		}

		/*
		 * send the compressed DataBurst to the libmarquise 
		 * poller thread
		 */

		//FIXME: IMPLEMENT
		

		/* Ack back to the client now we have accepted responsibility
		 * for delivery
		 */
		if (zmq_msg_send(&ident, zmq_listen_sock, ZMQ_SNDMORE) < 0) {
			perror("zmq_send (ident)");
		}
		else if(zmq_msg_send(&msg_id, zmq_listen_sock, ZMQ_SNDMORE) < 0) {
			perror("zmq_send (msg_id)");
		}
		else if (zmq_send(zmq_listen_sock, NULL, 0, 0) < 0) {
			perror("zmq_send (null ack)"), 1;
		}
		zmq_msg_close(&ident); 
		zmq_msg_close(&msg_id); 
		zmq_msg_close(&burst);

	}

	return 0;
}
