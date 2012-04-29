#include <avr/io.h>
#include <avr/interrupt.h>

#include "../protocol/serial.h"
#include "led_array.h"
#include "defines.h"

#define UBRR_VAL ((F_CPU+BAUDRATE*8)/(BAUDRATE*16)-1)
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUDRATE)
 
#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))
	#error Systematischer Fehler der Baudrate grösser 1% und damit zu hoch! 
#endif

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

	if (pos >= 2) {
		/* direct command; immediately send to handler */
		if (current_command.cmd.cmd == CMD_STREAM) {
			if (pos == 2) {
				/* reset stream on first byte */
				led_array_backbuffer_stream_rewind();
				pos++;
			}

			led_array_backbuffer_stream_write(in);
		} else {

			/* fixed command; length given due to structure */
			current_command.plain[pos++] = in;

			if (pos == sizeof(struct serial_command)) {
				/* handle static command */
				serial_command_handle(&current_command.cmd);
			}
		}
	} else {
		/* just input until command byte filled */
		current_command.plain[pos++] = in;
	}
}

void uart_init()
{
	/* set baudrate */
	UBRR0 = UBRR_VAL;
	
	/* enable tx + rx, rx interrupts */
	UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
	
}

ISR(USART_RX_vect)
{
//	uart_test();
	uart_handler(UDR0);
}

/**
 * uart_test() - set led 10/10 and swap buffer
 */
void uart_test()
{
	uint16_t i;
	uint8_t j, x, y;
	union serial_command_union test;

	test.cmd.header = 0x7F;
	test.cmd.cmd = CMD_SET;
	test.cmd.pos_x = 2;
	test.cmd.pos_y = 2;
	test.cmd.color = 3;

	for (y = 0; y < 16; y++) {
		for (x = 0; x < 96; x++) {
			//test.cmd.color = j;
			for (i = 0; i < sizeof(test.plain); i++)
				uart_handler(test.plain[i]);

			test.cmd.pos_x = x;
			test.cmd.pos_y = y;
			test.cmd.color = (x % 4) & ((y & 1) ? 3 : 0);
		}
//		test.cmd.pos_x++;
//		test.cmd.pos_y++;
	}

	test.cmd.cmd = CMD_SWAPBUFFER;
	for (i = 0; i < sizeof(test.plain); i++)
		uart_handler(test.plain[i]);
}
