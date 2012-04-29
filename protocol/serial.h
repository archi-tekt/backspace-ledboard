#include <stdint.h>

/* start byte 0x7F
 * escape for 0x7F and 0x7E
 * 0 = 0x7E, 1 = 0x7F */
#define SERIAL_ESCAPE_BYTE	0x7E
#define SERIAL_START	0x7F

#define BAUDRATE        115200

/**
 * enum commands - commands for serial communication protocol
 *
 * - modify buffer directly or after command complete?
 * - TODO: replies on success/error? / checksum needed?
 */
enum commands {
	CMD_SET,	/* set or clear a led */
	CMD_STREAM,	/* stream command for full led buffer */
	CMD_SWAPBUFFER  /* swap display/write buffer */
};

struct serial_command {
	uint8_t header; /* identification, always 0x7F */
	enum commands cmd : 8;
	uint8_t pos_x;
	uint8_t pos_y;
	uint8_t color;
} __attribute__((__packed__));

union serial_command_union {
	struct serial_command cmd;
	uint8_t plain[sizeof(struct serial_command)];
};
