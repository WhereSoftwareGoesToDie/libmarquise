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
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <endian.h>
#include <signal.h>
#include <assert.h>
#include <zmq.h>
#include <pthread.h>

#include "structs.h"
#include "defer.h"

#define DEFAULT_LISTEN_ADDRESS	"tcp://127.0.0.1:5560/"
#define POLLER_IPC_ADDRESS	"inproc://poller"

char do_shutdown;

/* most inefficient hexdump on the planet */
static void fhexdump(FILE *fp, uint8_t *buf, size_t bufsiz) {
	while (bufsiz--)
		fprintf(fp,"%02x", *(buf++));
}

extern void *marquise_poller(void *argsp);

/* Connect to the broker and start the libmarquise poller thread 
 * to handle talking to it
 *
 * returns a socket to talk to the poller on, or NULL on error
 */
void * start_marquise_poller(void *zmq_context, char *broker_socket_address) {
       	poller_args *pargs = malloc(sizeof(poller_args));
	if (pargs == NULL) return perror("malloc"), NULL;

	/* setup the broker's inbound IPC socket to avoid a race 
	 * This is only threadsafe because of the boundry on thread
	 * creation.
	 */
	pargs->collator_sock = zmq_socket(zmq_context, ZMQ_REP);
	if (zmq_bind(pargs->collator_sock, POLLER_IPC_ADDRESS)) 
		return perror("zmq_bind(" POLLER_IPC_ADDRESS ")"), NULL;

	/* and our end */
	void *poller_ipc_sock = zmq_socket(zmq_context, ZMQ_REQ);
	if (zmq_connect(poller_ipc_sock, POLLER_IPC_ADDRESS)) 
		return perror("zmq_connect(" POLLER_IPC_ADDRESS ")"), NULL;

	/* connect to broker */
	pargs->upstream_sock = zmq_socket(zmq_context, ZMQ_DEALER);
	if (zmq_connect(pargs->upstream_sock, broker_socket_address)) 
		return perror("zmq_bind"), NULL;

	/* no idea why we're the ones opening the deferral file */
	pargs->deferral_file = marquise_deferral_file_new();
	if (pargs->deferral_file == NULL) {
		zmq_close(pargs->upstream_sock);
		return perror("marquise_deferral_file_new"), NULL;
	}

	/* start the poller thread */
	pthread_t poller_pthread;
	if (pthread_create(&poller_pthread, NULL, marquise_poller, pargs)) {
		perror("pthread_create starting marquise poller");
		zmq_close(pargs->upstream_sock);
		return NULL;
	}
	return poller_ipc_sock;
}
static void shutdown_marquise_poller(void *zmq_context, void *poller_ipc_sock) {
	/* Send a blocking "DIE" message
	 * The poller won't ack until all messages are acked by the broker
	 */ 
	int ret;
	do {
		ret = zmq_send(poller_ipc_sock, "DIE", 3, 0);
	} while (ret == -1 && errno == EINTR);
	if (ret == -1) { perror("zmq_send"); return; }

	/* Block on poller flushing and shutting down */
	zmq_msg_t ack;
	zmq_msg_init(&ack);
	do {
		ret = zmq_recvmsg(poller_ipc_sock, &ack, 0);
	} while (ret == -1 && errno == EINTR);
	if (ret == -1) { perror("zmq_send"); return; }
	zmq_msg_close(&ack);

	/* Poller is done. Cleanup our end of the socket */
	zmq_close(poller_ipc_sock);
}

void marquised_shutdown(void *zmq_context, void *poller_ipc_sock, void *zmq_listen_sock) {
	printf("marquised shutting down.\n");
	shutdown_marquise_poller(zmq_context, poller_ipc_sock);
	zmq_close(zmq_listen_sock);
	zmq_ctx_destroy(zmq_context);
	exit(0);
}

void *handle_exit_signal(int sig) {
	printf("Caught fatal signal, cleaning up...\n");
	do_shutdown = 1;
}
		
int main(int argc, char **argv) {
	char *zmq_listen_address;
	sigset_t sig_mask;
	char *zmq_broker_address;
	void *zmq_listen_sock;
	void *zmq_context;
	void *poller_ipc_socket;
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
		
	/* Kick off zmq */
	zmq_context = zmq_ctx_new();

	/*
	 * Set up libmarquis' poller component to talk to the broker and
	 * deal with retries / disk deferral etc.
	 *
	 * We don't need the collation thread as frames are already grouped
	 * into DataBursts and compressed by the client
	 */

	poller_ipc_socket = start_marquise_poller(zmq_context, zmq_broker_address);
	if (poller_ipc_socket == NULL) return -1;

	/* 
	 * Bind the zmq socket to listen
	 */
	verbose_printf("binding listen socket to %s\n", zmq_listen_address);
	zmq_listen_sock = zmq_socket(zmq_context, ZMQ_ROUTER);
	if (zmq_bind(zmq_listen_sock, zmq_listen_address)) 
		return perror("zmq_bind"), 1;

	signal(SIGTERM, handle_exit_signal);
	signal(SIGHUP, handle_exit_signal);
	signal(SIGINT, handle_exit_signal);

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

		if (do_shutdown) {
			marquised_shutdown(zmq_context, poller_ipc_socket, zmq_listen_sock);
		}

		errno = 0;
		do { ident_rx  = zmq_msg_recv(&ident, zmq_listen_sock, 0);
		} while (errno == EINTR && !do_shutdown);
		if (ident_rx < 1 ) {
			perror("zmq_msg_recv (ident_rx)");
			shutdown_marquise_poller(zmq_context, poller_ipc_socket);
			zmq_close(zmq_listen_sock);
			zmq_ctx_destroy(zmq_context);
			return 1;
		}
		if (!zmq_msg_more(&ident)) {
			fprintf(stderr, "Got short message (only 1 part). Skipping.\n");
			zmq_msg_close(&ident);
			continue;
		}

		errno = 0;
		do { msg_id_rx = zmq_msg_recv(&msg_id, zmq_listen_sock, 0);
		} while (errno == EINTR && !do_shutdown);
		if (msg_id_rx < 1 ) {
			perror("zmq_msg_recv (msg_id_rx)");
			shutdown_marquise_poller(zmq_context, poller_ipc_socket);
			zmq_close(zmq_listen_sock);
			zmq_ctx_destroy(zmq_context);
			return 1;
		}
		if (!zmq_msg_more(&msg_id)) {
			fprintf(stderr, "Got short message (only 2 parts). Skipping.\n");
			zmq_msg_close(&ident); zmq_msg_close(&msg_id);
			continue;
		}

		errno = 0;
		do { burst_rx = zmq_msg_recv(&burst, zmq_listen_sock, 0);
		} while (errno == EINTR && !do_shutdown);
		if (burst_rx < 0 )  {
			perror("zmq_msg_recv (burst_rx)");
			shutdown_marquise_poller(zmq_context, poller_ipc_socket);
			zmq_close(zmq_listen_sock);
			zmq_ctx_destroy(zmq_context);
			return 1;
		}

		if (verbose) {
			fprintf(stderr, "received %lu bytes\n\tidentity:\t0x", zmq_msg_size(&burst));
			fhexdump(stderr, zmq_msg_data(&ident), zmq_msg_size(&ident));
			fprintf(stderr, "\n\tmessage id:\t0x");
			fhexdump(stderr, zmq_msg_data(&msg_id), zmq_msg_size(&msg_id));
			fputc('\n', stderr);
		}

		/*
		 * send the compressed DataBurst to the libmarquise 
		 * poller thread.
		 */
		zmq_msg_t msg;
		zmq_msg_init(&msg);
		zmq_msg_copy(&msg, &burst); 
		int ret;
		do {
			ret = zmq_sendmsg(poller_ipc_socket, &msg, 0);
		} while (ret == -1 && errno == EINTR && !do_shutdown);
		if (ret == -1) {
			perror("zmq_sendmsg (to poller)");
			shutdown_marquise_poller(zmq_context, poller_ipc_socket);
			zmq_close(zmq_listen_sock);
			zmq_ctx_destroy(zmq_context);
			return 1;
		}

		/* Wait for ack from poller */
		zmq_msg_t ack;
		zmq_msg_init(&ack);
		do {
			ret = zmq_msg_recv(&ack, poller_ipc_socket, 0);
		} while (ret == -1 && errno == EINTR && !do_shutdown);
		if (ret == -1) {
			perror("zmq_msg_recv (waiting on internal ack from poller)");
			shutdown_marquise_poller(zmq_context, poller_ipc_socket);
			zmq_close(zmq_listen_sock);
			zmq_ctx_destroy(zmq_context);
			return 1;
		}
		zmq_msg_close(&ack);

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
	marquised_shutdown(zmq_context, poller_ipc_socket, zmq_listen_sock);
	return 0;
}
