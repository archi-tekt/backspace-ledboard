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

#include <libedboard.h>

struct matrix_item {
	int is_space;
	unsigned int length;
};

void matrix_item_rand(struct matrix_item *item)
{
	item->is_space ^= 1;
	item->length = rand() % ARRAY_Y_SIZE + 1; /* minimum length: 1 */
}

int main(int argc, char *argv[])
{
	struct matrix_item matrix[ARRAY_X_SIZE];
	uint8_t frame[ARRAY_Y_SIZE][ARRAY_X_SIZE];
	struct timespec ts;
	int sockfd;
	int i;

	if (argc != 2) {
		puts("Usage: ledmatrix <ledloard host>");
		exit(1);
	}
	if ((sockfd = ledboard_connect(argv[1])) == -1) {
		exit(1);
	}

	memset(frame, 0, sizeof(frame));

	ts.tv_sec = 0;
	ts.tv_nsec = 150 * 1000 * 1000;
	/* initialize matrix with random type */
	for (i = 0; i < ARRAY_X_SIZE; ++i) {
		matrix[i].is_space = rand() % 2;
		matrix[i].length = 0;
	}
	for (;;) {
		/* scroll rows down */
		memmove(frame[1], frame[0], (ARRAY_Y_SIZE - 1) * ARRAY_X_SIZE);

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
			frame[0][i] = color;
			matrix[i].length--;
		}
		if (ledboard_send_raw(sockfd, frame) == -1) {
			perror("ledboard_send_frame");
		}
		nanosleep(&ts, NULL);
	}
	close(sockfd);
	return 0;
}
