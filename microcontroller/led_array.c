#define ARRAY_X_SIZE	96
#define ARRAY_Y_SIZE	16

#include "defines.h"
#include <avr/io.h>
#include <avr/interrupt.h>

/*
 * why macros, see:
 * http://projects.bckspc.de/trac/ledboard/wiki/StructMacroInlineConsiderations
 */

#define OUTPUT_ENABLE(on)	{if(on) OUTPUT_SET(OUTPUT_PORT_ENABLE, OUTPUT_PIN_ENABLE ); \
	else OUTPUT_CLEAR(OUTPUT_PORT_ENABLE, OUTPUT_PIN_ENABLE);}
#define OUTPUT_CLOCK(on)	{if(on) OUTPUT_SET(OUTPUT_PORT_CLOCK, OUTPUT_PIN_CLOCK); \
	else OUTPUT_CLEAR(OUTPUT_PORT_CLOCK, OUTPUT_PIN_CLOCK);}
#define OUTPUT_STORE_TOGGLE()	(OUTPUT_TOGGLE(OUTPUT_PORT_STORE, OUTPUT_PIN_STORE))

enum draw_type {
	DRAW_TYPE_SET,
	DRAW_TYPE_CLEAR,
	DRAW_TYPE_TOGGLE
};

/*
 * buffer arrangement:
 * - with 4 colors, we need 2 bits per pixel
 * - right side of board is where µC is located
 * - buffer starts with top (row 0) most left pixel (2 bits)
 * - next is the second pixel from left (still row 0)
 * 
 * - double buffering, data is written to backbuffer while read
 *   is done on frontbuffer
 * 
 * TODO: buffer needs to be read-accessed very fast, if needed -> rearrange
 */
static uint8_t buffer[2][ARRAY_X_SIZE * ARRAY_Y_SIZE * 2 / 8];
static uint8_t active_buffer;

/*
 * switch backbuffer to frontend buffer
 */
void led_array_swap_buffer()
{
	active_buffer ^= 1;
}

/*
 * set a LED in backbuffer
 */
void led_array_backbuffer_bit_set(uint8_t x, uint8_t y, uint8_t color)
{
	uint16_t y_offset = y * ARRAY_X_SIZE * 2 / 8;
	uint8_t x_offset = x * 2 / 8;
	uint8_t x_shift = (6 - ((x * 2) % 8));

	uint8_t val = buffer[active_buffer^1][y_offset + x_offset];
	val &= ~(3 << x_shift);
	val |= (color & 3) << x_shift;

	buffer[active_buffer^1][y_offset + x_offset] = val;
}


static uint8_t greyscale_counter;

/*
 * set bit in shift register
 * output is low active
 */
static __attribute__((always_inline)) void led_array_output_bit(uint8_t color)
{
	if (greyscale_counter < color)
		OUTPUT_CLEAR(OUTPUT_PORT_BITS, OUTPUT_PIN_BITS);
	else
		OUTPUT_SET(OUTPUT_PORT_BITS, OUTPUT_PIN_BITS);

}

static __attribute__((always_inline)) void led_array_output_line(uint8_t line)
{
	OUTPUT_PORT_LINE = (OUTPUT_PORT_LINE & ~OUTPUT_MASK_LINE) |
		(line & OUTPUT_MASK_LINE);

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
	led_array_output_line(0);

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
#if 0
			if (y == 2)
				/* 0% grey */
				led_array_output_bit(0);
			else if (y == 3)
				/* 33% grey*/
				led_array_output_bit(1);
			else if (y == 4)
				/* 66% grey */
				led_array_output_bit(2);
			else
				/* 100% red :) */
				led_array_output_bit(3);
#endif
				led_array_output_bit( y );


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
		led_array_output_line(y);

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
	greyscale_counter++;

	/* TODO: check if %4 is more efficient */
	if ( greyscale_counter == 16 )
		greyscale_counter = 0;

	/* disable output; fail? is disabled already? */
	OUTPUT_ENABLE(0);

	//TIMSK0 = 1;
}
