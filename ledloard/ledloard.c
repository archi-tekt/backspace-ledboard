#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "../protocol/serial.h"

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
	if (tcsetattr(fd, TCSANOW, &new_term) == -1)
		return -1;
	return 0;
}

int main(int argc, char *argv[])
{
	int led_fd;

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

	return 0;
}
