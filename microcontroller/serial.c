#include <avr/io.h>

/* start byte 0x7F */
/* escape for 0x7F and 0x7E */
/* 0 = 0x7E, 1 = 0x7F */

#define SERIAL_ESCAPE_BYTE	0x7E
#define SERIAL_START	0x7F

/* commands:
	0) direct commands
	1) fill complete led board complete
	2) swap buffers
	3) fill one line
	4) bit set/clear/toggle
	5) invert
	6) 
	
	modify buffer directly or after command complete?
	TODO: backbuffer stream mode (less overhead for calc pos
 *		and serial data)
 *	TODO: replies on success/error? / checksum needed?
 */

enum commands {
	CMD_SET = 0,	/* set or clear a led */
	CMD_SWAPBUFFER, /* swap display/write buffer */
	CMD_TOGGLE,	/* toggle led */
	CMD_INVERT	/* invert write buffer */
};

struct serial_command {
	uint8_t header; /* identification, always 0x7F */
	enum commands cmd : 8;
	uint8_t direct_draw;
	/* position of leds to change */
	uint8_t pos_x;
	uint8_t pos_y;
	/* length of data only, not including escape chars */
	uint16_t data_length;
} __attribute__((__packed__));

union serial_command_union {
	struct serial_command cmd;
	uint8_t plain[sizeof( struct serial_command) ];
};

static uint8_t header_pos;
static uint8_t data_pos;
static uint8_t escape_next;
static union serial_command_union current_command;

void uart_handler(uint8_t in)
{

	if (in == SERIAL_START) {
		header_pos = 0;
		data_pos = 0;
		return;
	}

	if (in == SERIAL_ESCAPE_BYTE) {
		escape_next = 1;
		return;
	}

	if (escape_next) {
		in ^= SERIAL_ESCAPE_BYTE;
		escape_next = 0;
	}


	if (header_pos == sizeof( struct serial_command)) {
		/* incoming data */
		data_pos++;
		if (current_command.cmd.data_length == data_pos) {
			/* end reached of command -> handle */
		}
	} else {
		/* incoming header */
		current_command.plain[header_pos++] = in;
	}

}

void uart_ISR()
{
	uart_handler(0);
}

void uart_test()
{
	uint16_t i;
	const uint8_t test[] = {0x7F, 0x00};

	for (i = 0; i < sizeof( test); i++)
		uart_handler(test[i]);
}

