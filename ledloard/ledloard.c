#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <libedboard.h>
#include "../ledboard/defines.h"

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

int serial_write(int fd, const struct ledboard_command *cmd)
{
	static uint8_t led_buffer[ARRAY_Y_SIZE][ARRAY_X_SIZE * 2 / 8];
	uint8_t ack;
	int i, j, k;

	for (i = 0; i < ARRAY_Y_SIZE; ++i) {
		int frame_index = 0;
		for (j = 0; j < ARRAY_X_SIZE * 2 / 8; ++j) {
			uint8_t byte = 0;
			for (k = 0; k < 4; ++k) {
				byte |= (cmd->frame[i][frame_index++] & 0x03) << (k * 2);
			}
			led_buffer[i][j] = byte;
		}
	}

	if (write(fd, led_buffer, sizeof(led_buffer)) != sizeof(led_buffer))
		return -1;
	if (read(fd, &ack, sizeof(ack)) != ack)
		return -1;
	/* check if it's 0xFE */
	return 0;
}

/**
 * ledloard_server() - initialize server socket
 * @return:	server socket, -1 for error
 */
int ledloard_server(void)
{
	int ret;
	int one = 1;
	int sockfd;
	struct addrinfo hints, *res, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((ret = getaddrinfo(NULL, "1337", &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		return -1;
	}

	for (p = res; p; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1)
			continue;
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == 0)
			break;
		close(sockfd);
	}
	freeaddrinfo(res);
	if (p == NULL) {
		fprintf(stderr, "unable to bind\n");
		return -1;
	}

	if (listen(sockfd, 1) == -1) {
		perror("listen");
		return -1;
	}
	return sockfd;
}

int main(int argc, char *argv[])
{
	int led_fd, sock_fd, conn_fd;
	uint8_t type;
	struct ledboard_command recv_buffer;
	ssize_t recvd;

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

	if ((sock_fd = ledloard_server()) == -1) {
		exit(1);
	}

	for (;;) {
		if ((conn_fd = accept(sock_fd, NULL, NULL)) == -1) {
			perror("Unable to accept connection");
			continue;
		}
		for (;;) {
			recvd = recv(conn_fd, &recv_buffer, sizeof(recv_buffer), MSG_WAITALL);
			if (recvd == 0)
				break;
			if (recvd != sizeof(recv_buffer))
				perror("recv");
			serial_write(led_fd, &recv_buffer);
			puts("frame written");
		}
		close(conn_fd);
	}
	close(sock_fd);
	close(led_fd);
	return 0;
}
