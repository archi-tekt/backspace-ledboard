#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "../ledboard/protocol/config.h"

struct command {
	uint8_t type;
	uint8_t frame[ARRAY_Y_SIZE][ARRAY_X_SIZE];
} __attribute__((packed));

struct matrix_item {
	int is_space;
	unsigned int length;
};

void matrix_item_rand(struct matrix_item *item)
{
	item->is_space ^= 1;
	item->length = rand() % ARRAY_Y_SIZE + 1; /* minimum length: 1 */
}

int ledloard_connect(const char *host)
{
	struct addrinfo hints, *res, *p;
	int sockfd;
	int ret;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((ret = getaddrinfo(host, "1337", &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		return -1;
	}
	for (p = res; p; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0)
			break;
		close(sockfd);
	}
	freeaddrinfo(res);
	if (p == NULL) {
		fprintf(stderr, "Unable to connect to %s\n", host);
		return -1;
	}
	return sockfd;
}

int main(int argc, char *argv[])
{
	struct matrix_item matrix[ARRAY_X_SIZE];
	struct command cmd;
	struct timespec ts;
	int sockfd;
	int i;

	if (argc != 2) {
		puts("Usage: ledmatrix <ledloard host>");
		exit(1);
	}
	if ((sockfd = ledloard_connect(argv[1])) == -1) {
		exit(1);
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.type = 0x00; /* send raw frame */

	ts.tv_sec = 0;
	ts.tv_nsec = 150 * 1000 * 1000;
	/* initialize matrix with random type */
	for (i = 0; i < ARRAY_X_SIZE; ++i) {
		matrix[i].is_space = rand() % 2;
		matrix[i].length = 0;
	}
	for (;;) {
		/* scroll rows down */
		memmove(cmd.frame[1], cmd.frame[0], (ARRAY_Y_SIZE - 1) * ARRAY_X_SIZE);

		/* fill first row */
		for (i = 0; i < ARRAY_X_SIZE; ++i) {
			bool first = false;
			uint8_t color;
			if (matrix[i].length == 0) {
				matrix_item_rand(&matrix[i]);
				first = true;
			}
			if (matrix[i].is_space) {
				color = 0; /* dark */
			} else if (first) {
				color = 3; /* brightest */
				first = false;
			} else {
				color = rand() % 2 + 1;
			}
			cmd.frame[0][i] = color;
			matrix[i].length--;
		}
		if (send(sockfd, &cmd, sizeof(cmd), 0) != sizeof(cmd))
			perror("send");
		nanosleep(&ts, NULL);
	}
	close(sockfd);
	return 0;
}
