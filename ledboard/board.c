#include <avr/io.h>
#include <avr/interrupt.h>
#include "../libedboard/libedboard.h"
#include "led_array.h"

#include "defines.h"

void init()
{
	/* disable interrupts */
	cli();

	/* set outputs */
	/* LINE_A */
	DDRC |= 1;

	/* LINE_B */
	DDRC |= 1 << 1;

	/* LINE C */
	DDRC |= 1 << 2;

	/* LINE_D */
	DDRC |= 1 << 3;

	/* HE - output enable */
	DDRC |= 1 << 4;

	/* RCK - TODO */
	DDRC |= 1 << 5;

	/* R0 - bits */
	DDRD |= 1 << 4;

	/* SCK */
	DDRB |= 1 << 1;

#if 0
	/* setup timer 0 */
	/* prescaler 256 */
	TCCR0B = 4;

	/* irq directly after */
	TCNT0 = 0xFF;

	TIMSK0 = 1;
#endif

	sei();
}

int main()
{
	/* init hw */
	init();
	uart_init();

	/* sent one ack in the beginnging for power up scenarios */
	UDR0 = ACK_BYTE;

	/* remaining stuff is irq driven */
//	uart_test();
	while (1) {
		led_array_draw();
		//UDR0 = 0xFF;
	}

	return 0;
}
