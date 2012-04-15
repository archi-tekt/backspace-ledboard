#include <avr/io.h>
#include "led_array.h"

/* start byte 0x7F
 * escape for 0x7F and 0x7E
 * 0 = 0x7E, 1 = 0x7F */
#define SERIAL_ESCAPE_BYTE	0x7E
#define SERIAL_START	0x7F

/* commands:
 *	0) direct commands
 *	1) swap buffers
 *	modify buffer directly or after command complete?
 *	TODO: backbuffer stream mode (less overhead for calc pos
 *		and serial data)
 *	TODO: replies on success/error? / checksum needed?
 */
enum commands {
	CMD_SET,	/* set or clear a led */
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

/**
 * serial_command_handle() - handle incoming command
 * cmd:		the command to handle
 */
static void serial_command_handle(const struct serial_command *cmd)
{
	switch (cmd->cmd) {
	case CMD_SET:
		led_array_backbuffer_bit_set(cmd->pos_x, cmd->pos_y, cmd->color);
		break;
	case CMD_SWAPBUFFER:
		led_array_swap_buffer();
		break;
	default:
		break;
	}
}

/**
 * uart_handler() - uart handler
 * @in:		incoming byte
 */
void uart_handler(uint8_t in)
{
	static union serial_command_union current_command;
	static uint8_t escape_next;
	static uint8_t pos;

	if (in == SERIAL_START) {
		pos = 0;
	}

	if (in == SERIAL_ESCAPE_BYTE) {
		escape_next = 1;
		return;
	}

	if (escape_next) {
		in ^= SERIAL_ESCAPE_BYTE;
		escape_next = 0;
	}

	current_command.plain[pos++] = in;
	if (pos == sizeof(struct serial_command)) {
		/* handle command */
		serial_command_handle(&current_command.cmd);
	}
}

void uart_ISR()
{
	uart_handler(0);
}

/**
 * uart_test() - set led 10/10 and swap buffer
 */
void uart_test()
{
	uint16_t i;
	uint8_t j;
	union serial_command_union test;

	test.cmd.header = 0x7F;
	test.cmd.cmd = CMD_SET;
	test.cmd.pos_x = 2;
	test.cmd.pos_y = 2;
	test.cmd.color = 3;

	for (j = 0; j < 4; j++) {
		test.cmd.color = j;
		for (i = 0; i < sizeof(test.plain); i++)
			uart_handler(test.plain[i]);
		test.cmd.pos_x++;
		test.cmd.pos_y++;
	}

	test.cmd.cmd = CMD_SWAPBUFFER;
	for (i = 0; i < sizeof(test.plain); i++)
		uart_handler(test.plain[i]);
}
