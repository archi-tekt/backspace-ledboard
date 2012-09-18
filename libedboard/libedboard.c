#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include "libedboard.h"

int ledboard_connect(const char *host)
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

static ssize_t send_all(int fd, const void *buf, size_t len)
{
	ssize_t n;
	while (len > 0) {
		n = send(fd, buf, len, 0);
		if (n == -1)
			return -1;
		buf += n;
		len -= n;
	}
	return 0;
}

int ledboard_send_priority(int fd, enum ledboard_priority prio)
{
	uint8_t cmd[2];

	if (prio < LB_PRIO_NORMAL || prio > LB_PRIO_GOD) {
		errno = EINVAL;
		return -1;
	}
	cmd[0] = LB_TYPE_PRIO;
	cmd[1] = prio;
	return send_all(fd, &cmd, sizeof(cmd));
}

int ledboard_send_raw(int fd, uint8_t frame[ARRAY_Y_SIZE][ARRAY_X_SIZE])
{
	uint8_t cmd;
	uint8_t ready;

	/* block until we've read the ready byte */
	if (read(fd, &ready, 1) != 1)
		return -1;
	if (ready != LB_TYPE_READY)
		return -1;

	cmd = LB_TYPE_RAW;
	if (send_all(fd, &cmd, sizeof(cmd)) == -1)
		return -1;
	return send_all(fd, frame, sizeof(uint8_t) * ARRAY_Y_SIZE * ARRAY_X_SIZE);
}

int ledboard_send_pixel(int fd, uint8_t x, uint8_t y, enum ledboard_color color)
{
	uint8_t cmd[4];

	if (x >= ARRAY_X_SIZE || y >= ARRAY_X_SIZE || color > 3) {
		errno = EINVAL;
		return -1;
	}
	cmd[0] = LB_TYPE_SETXY;
	cmd[1] = x;
	cmd[2] = y;
	cmd[3] = color;
	return send_all(fd, cmd, sizeof(cmd));
}
