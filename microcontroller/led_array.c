#define ARRAY_X_SIZE	96
#define ARRAY_Y_SIZE	16

#include "defines.h"
#include <avr/io.h>
#include <avr/interrupt.h>

/* why macros, see:
 * http://projects.bckspc.de/trac/ledboard/wiki/StructMacroInlineConsiderations
 */

#define OUTPUT_LINE_SET(line)	(OUTPUT_PORT_LINE = ((OUTPUT_PORT_LINE & \
	~OUTPUT_MASK_LINE) | (line & OUTPUT_MASK_LINE))) 
#define OUTPUT_ENABLE(on)	{if(on) OUTPUT_SET(OUTPUT_PORT_ENABLE, OUTPUT_PIN_ENABLE ); \
	else OUTPUT_CLEAR(OUTPUT_PORT_ENABLE, OUTPUT_PIN_ENABLE);}
#define OUTPUT_CLOCK(on)	{if(on) OUTPUT_SET(OUTPUT_PORT_CLOCK, OUTPUT_PIN_CLOCK); \
	else OUTPUT_CLEAR(OUTPUT_PORT_CLOCK, OUTPUT_PIN_CLOCK);}
/* bits are low active */
#define OUTPUT_BITS(on)		{if(on) OUTPUT_CLEAR(OUTPUT_PORT_BITS, OUTPUT_PIN_BITS); \
	else OUTPUT_SET(OUTPUT_PORT_BITS, OUTPUT_PIN_BITS);}
#define OUTPUT_STORE_TOGGLE()	(OUTPUT_TOGGLE(OUTPUT_PORT_STORE, OUTPUT_PIN_STORE))

enum draw_type {
	DRAW_TYPE_SET,
	DRAW_TYPE_CLEAR,
	DRAW_TYPE_TOGGLE
};

static uint8_t buffer[2][ARRAY_X_SIZE / 8 * ARRAY_Y_SIZE ];
static uint8_t active_buffer;

void led_array_swap_buffer()
{
}

void led_array_bit_set(uint8_t x, uint8_t y, enum draw_type type)
{

}

void led_array_data_set(uint8_t x, uint8_t y, uint8_t data)
{

}

void led_array_data_toggle(uint8_t x, uint8_t y, uint8_t data)
{

}

/*
ISR (TIMER0_OVF_vect)
 */
void led_array_all_on()
{
	uint8_t y, x, j;
	//unsigned short shift;
	//cli();

#if 0
	TIMSK0 = 0;
	//sei();

	TCNT0 = 0xd5;
#endif
	/* disable output */
	OUTPUT_ENABLE(0);

	/* set line 0 */
	OUTPUT_LINE_SET(0);

	for (y = 0; y < ARRAY_Y_SIZE; y++) {
		//shift = 1 << y;
		for (x = 0; x < ARRAY_X_SIZE; x++) {
			/* clock low */
			OUTPUT_CLOCK(0);
#if 0
			/* update bits */
			if (frame_buffer[t] & shift)
				OUTPUT_CLEAR(PORTD, 4);
			else
				OUTPUT_SET(PORTD, 4);
#endif
			OUTPUT_BITS(1);

			/* clock high */
			OUTPUT_CLOCK(1);

		}

		/* enable output */
		OUTPUT_ENABLE(1);

		/* let the leds bright for some time */
		ASM_DELAY(j, 10);

		/* save? str */
		OUTPUT_STORE_TOGGLE();
		OUTPUT_STORE_TOGGLE();

		/* change line; if not done here old line is still bright a bit */
		OUTPUT_LINE_SET(y);

		/* disable output */
		OUTPUT_ENABLE(0);

		ASM_DELAY(j, 10);
		ASM_DELAY(j, 10);
	}

#if 0
	/* change buffers */
	if (update_request) {
		update_request = 0;
		for (i = 0; i < 96; i++)
			frame_buffer[i] = frame[i];
	}
#endif
	ASM_DELAY(j, 255);

	/* disable output */
	OUTPUT_ENABLE(0);

	//TIMSK0 = 1;
}
