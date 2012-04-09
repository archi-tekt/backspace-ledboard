#include <avr/io.h>
#include <avr/interrupt.h>
//#include "serial.h"

void init ()
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

	/* setup timer 0 */
	/* prescaler 256 */
	TCCR0B = 4;

	/* irq directly after */
	TCNT0 = 0xFF;

	TIMSK0 = 1;

	sei();
}

int main()
{
	int i;

	/* init hw */
	init();

	/* remaining stuff is irq driven */
	while (1);

	for (i = 0; i < 10; ++i) {
		asm volatile("nop");
	}

	return 0;
}

