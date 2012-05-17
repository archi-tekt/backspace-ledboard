#ifndef LIBEDBOARD_H
#define LIBEDBOARD_H

#define ARRAY_X_SIZE 96
#define ARRAY_Y_SIZE 16

struct ledboard_command {
	uint8_t type;
	uint8_t frame[ARRAY_Y_SIZE][ARRAY_X_SIZE];
} __attribute__((packed));

/**
 * ledboard_connect() - connect to ledloard host
 * @host:        ledloard hostname/ip
 *
 * The function returns the file descriptor ready for reading/writing
 * or -1 in case of error (error messages already written to stderr)
 */
int ledboard_connect(const char *host);

#endif
