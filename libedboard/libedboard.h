#ifndef LIBEDBOARD_H
#define LIBEDBOARD_H

#define ARRAY_X_SIZE 96
#define ARRAY_Y_SIZE 16

enum ledboard_priority {
	LB_PRIO_NORMAL = 0xF0,
	LB_PRIO_URGENT,
	LB_PRIO_GOD
};

enum ledboard_color {
	LB_COLOR_OFF,
	LB_COLOR_LIGHT,
	LB_COLOR_MIDDLE,
	LB_COLOR_BRIGHT
};

enum ledboard_types {
	LB_TYPE_RAW = 0xA1,
	LB_TYPE_PRIO,
	LB_TYPE_SETXY,
	LB_TYPE_ASCII
};

/**
 * ledboard_connect() - connect to ledloard host
 * @host:        ledloard hostname/ip
 *
 * The function returns the socket descriptor ready for reading/writing
 * or -1 for error (error messages already written to stderr!)
 */
int ledboard_connect(const char *host);

/**
 * ledboard_send_priority() - set priority
 */
int ledboard_send_priority(int fd, enum ledboard_priority prio);

/**
 * ledboard_send_raw() - set a raw frame
 */
int ledboard_send_raw(int fd, uint8_t frame[ARRAY_Y_SIZE][ARRAY_X_SIZE]);

/**
 * ledboard_send_pixel() - set a single pixel
 * @x:		x coordinate (0..ARRAY_X_SIZE-1)
 * @y:		y coordinate (0..ARRAY_Y_SIZE-1)
 */
int ledboard_send_pixel(int fd, uint8_t x, uint8_t y, enum ledboard_color color);
#endif
