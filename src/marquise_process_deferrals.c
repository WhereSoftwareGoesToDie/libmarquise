#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include "macros.h"
#include "marquise.h"
#include "defer.h"

#define POLLER_ADDRESS "inproc://poller"

extern void *marquise_poller(void *argsp);

void *start_poller(void *zmq_ctx, const char *broker) 
{
	poller_args *pargs = malloc(sizeof(poller_args));
	if (pargs == NULL) {
		perror("malloc");
		return NULL;
	}
	pargs->collator_sock = zmq_socket(zmq_ctx, ZMQ_REP);
	if (zmq_bind(pargs->collator_sock, POLLER_ADDRESS) != 0) {
		perror("zmq_bind");
		return NULL;
	}
	void *poller_sock = zmq_socket(zmq_ctx, ZMQ_REQ);
	if (zmq_connect(poller_sock, POLLER_ADDRESS) != 0) {
		perror("zmq_connect");
		return NULL;
	}
	pargs->upstream_sock = zmq_socket(zmq_ctx, ZMQ_DEALER);
	if (zmq_connect(pargs->upstream_sock, broker) != 0) {
		perror("zmq_connect");
		return NULL;
	}

	pargs->deferral_file = marquise_deferral_file_new();
	if (pargs->deferral_file == NULL) {
		zmq_close(pargs->upstream_sock);
		perror("marquise_deferral_file_new");
		return NULL;
	}	
	pthread_t poller_thread;
	if (pthread_create(&poller_thread, NULL, marquise_poller, pargs) != 0) {
		perror("pthread_create");
		zmq_close(pargs->upstream_sock);
		return NULL;
	}
	return poller_sock;
}

void shutdown_poller(void *zmq_ctx, void *poller_sock) {
	int ret;
	do {
		ret = zmq_send(poller_sock, "DIE", 3, 0);
	} while (ret == -1 && errno == EINTR);
	if (ret == -1) {
		perror("zmq_send");
		return;
	}

	zmq_msg_t ack;
	zmq_msg_init(&ack);
	do {
		ret = zmq_recvmsg(poller_sock, &ack, 0);
	} while (ret == -1 && errno == EINTR);
	if (ret == -1) {
		perror("zmq_recvmsg");
		return;
	}
	zmq_msg_close(&ack);
	zmq_close(poller_sock);
}	

void shutdown(void *zmq_ctx, void *poller_sock) {
	printf("Finished. Cleaning up.\n");
	shutdown_poller(zmq_ctx, poller_sock);
	zmq_ctx_destroy(zmq_ctx);
	exit(0);
}

void send_burst(void *zmq_ctx, void *poller_sock, data_burst *burst) {
	int ret;
	do {
		ret = zmq_send(poller_sock, burst->data, burst->length, 0);
	} while (ret == -1 && errno == EINTR);
	if (ret == -1) {
		perror("zmq_sendmsg");
		return;
	}
	zmq_msg_t ack;
	zmq_msg_init(&ack);
	do {
		ret = zmq_msg_recv(&ack, poller_sock, 0);
	} while (ret == -1 && errno == EINTR);
	if (ret == -1) {
		perror("zmq_msg_recv (poller ack)");
		return;
	}
	zmq_msg_close(&ack);
}

void process_defer_file(char *path) 
{
	int fd = open(path, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Could not open %s.\n", path);
		return;
	}
	if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
		if (errno == EWOULDBLOCK) {
			fprintf(stderr, "%s is locked, skipping.\n", path);
			flock(fd, LOCK_UN);
			close(fd);
		}
		fprintf(stderr, "Could not lock %s: %s\n", path, strerror(errno));
		flock(fd, LOCK_UN);
		close(fd);
	}
	struct stat buf;
	if (fstat(fd, &buf) != 0) {
		fprintf(stderr, "Could not stat %s: %s\n", path, strerror(errno));
		flock(fd, LOCK_UN);
		close(fd);
		return;
	}
	if (buf.st_size == 0) {
		printf("Deleting zero-size %s\n", path);
		unlink(path);
		return;
	}
	// Of nonzero size and not locked, so we retry it
	printf("Retrying %s...\n", path);
	FILE *fi = fdopen(fd, "r");
	if (fi == NULL) {
		perror("fdopen");
		flock(fd, LOCK_UN);
		close(fd);
	}
	deferral_file *df = malloc(sizeof(deferral_file));
	df->path = malloc(strlen(path));
	strcpy(df->path, path);
	df->stream = fi;
	df->fd = fd;
	data_burst *burst;
	int retrieved = 0;
	for (;;) {
		printf("fd: %d\n", df->fd);
		burst = marquise_retrieve_from_file(df);
		if (!burst) {
			break;
		}
		retrieved++;
	}
	printf("Retrieved %d bursts from %s.\n", retrieved, path);
		
	marquise_deferral_file_close(df);
	marquise_deferral_file_free(df);
}

int main(int argc, char **argv) 
{
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <defer_dir> <broker>\n", argv[0]);
		exit(1);
	}
	int template_len = strlen(deferral_file_template);
	// We want to cut off the random suffix at the end. 
	size_t prefix_len = template_len - 6;
	char *defer_prefix = malloc(template_len);
	// Same with the / at the start.
	strncpy(defer_prefix, &deferral_file_template[1], prefix_len);
	defer_prefix[prefix_len-1] = '\0';
	size_t defer_dir_len = strlen(argv[1]);
	void *zmq_ctx = zmq_ctx_new();
	void *poller_sock = start_poller(zmq_ctx, argv[2]);
	DIR *defer_dir = opendir(argv[1]);
	if (defer_dir == NULL) {
		fprintf(stderr, "Could not open deferral directory %s: %s\n", argv[1], strerror(errno));
		exit(2);
	}
	struct dirent *entry = NULL;
	for (;;) {
		errno = 0;
		entry = readdir(defer_dir);
		if (errno) {
			fprintf(stderr, "Error reading directory entry: %s\n", strerror(errno));
			break;
		}
		if (entry == NULL) {
			break; // eof
		}
		size_t name_len = strlen(entry->d_name);
		if (strncmp(entry->d_name, defer_prefix, prefix_len-1) != 0) {
			continue;
		}
		char *path = malloc(name_len * defer_dir_len + 2);
		strcpy(path, argv[1]);
		path[defer_dir_len] = '/';
		path[defer_dir_len + 1] = '\0';
		strncat(path, entry->d_name, name_len);
		process_defer_file(path);
		free(path);
	}
	free(defer_prefix);
	shutdown(zmq_ctx, poller_sock);
	return 0;
}
