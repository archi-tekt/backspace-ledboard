#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "../protocol/config.h"

/* create termios.h B<rate> constant from serial.h BAUDRATE */
#define termios_baud_(rate) B ## rate
#define termios_baud(rate) termios_baud_(rate)

static struct termios old_term;

/**
 * serial_init() - initialize serial device
 * @fd:		open file descriptor
 * @return:	0 for success, -1 for error
 */
int serial_init(int fd)
{
	struct termios new_term;

	/* save old terminal attributes */
	if (tcgetattr(fd, &old_term) == -1)
		return -1;

	memset(&new_term, 0, sizeof(new_term));
	/* set baud rates */
	if (cfsetospeed(&new_term, termios_baud(BAUDRATE)) == -1)
		return -1;
	if (cfsetispeed(&new_term, termios_baud(BAUDRATE)) == -1)
		return -1;
	/* set non-canonical mode */
	new_term.c_lflag &= ~ICANON;
	/* wait for 1 byte to arrive */
	new_term.c_cc[VMIN] = 1;
	if (tcsetattr(fd, TCSANOW, &new_term) == -1)
		return -1;
	return 0;
}

/**
 * net_init() - initialize server socket
 * @return:	server socket, -1 for error
 */
int net_init()
{
	int ret;
	int sockfd;
	struct addrinfo hints, *res, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if ((ret = getaddrinfo(NULL, "1337", &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		return -1;
	}

	for (p = res; p; p = res->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == 0)
			break;
		close(sockfd);
	}
	if (p == NULL) {
		fprintf(stderr, "Unable to bind\n");
		return -1;
	}

	freeaddrinfo(res);
	if (listen(sockfd, 1) == -1)
		return -1;
	return sockfd;
}

int main(int argc, char *argv[])
{
	uint8_t ack_buf;
	uint8_t led_buffer[ARRAY_Y_SIZE][ARRAY_X_SIZE * 2 / 8];
	uint8_t display_byte = 0;
	int led_fd, sock_fd;
	int i, j;

	if (argc != 2) {
		puts("Usage: ledlord <serial device>");
		exit(1);
	}

	if ((led_fd = open(argv[1], O_RDWR | O_NOCTTY)) == -1) {
		fprintf(stderr, "Unable to open %s: %s\n",
				argv[1], strerror(errno));
		exit(1);
	}

	if (serial_init(led_fd) == -1) {
		fprintf(stderr, "Unable to initialize serial device %s: %s\n",
				argv[1], strerror(errno));
		exit(1);
	}

	if ((sock_fd = net_init()) == -1) {
		perror("Unable to initialize server socket");
		exit(1);
	}

	if (accept(sock_fd, NULL, NULL) == -1) {
		perror("Unable to accept connection");
		exit(1);
	}
	puts("yay, client connected!");
	close(sock_fd);
	close(led_fd);
	return 0;
}
