#include <avr/io.h>
#include <avr/interrupt.h>

#include "../protocol/config.h"
#include "led_array.h"
#include "defines.h"

#define UBRR_VAL ((F_CPU+BAUDRATE*8)/(BAUDRATE*16)-1)
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUDRATE)
 
#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))
	#error Systematischer Fehler der Baudrate grösser 1% und damit zu hoch! 
#endif

/**
 * uart_handler() - uart handler
 * @in:		incoming byte
 */
void uart_handler(uint8_t in)
{
	/* check if backbuffer is free for writing */
	if (!led_array_backbuffer_free())
		return;

	led_array_backbuffer_stream_write(in);
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

#if 0
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
#endif
