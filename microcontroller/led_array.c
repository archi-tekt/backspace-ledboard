#include <avr/io.h>
#include <avr/interrupt.h>
#include "led_array.h"
#include "defines.h"

#define ARRAY_X_SIZE 96
#define ARRAY_Y_SIZE 16

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
static uint8_t buffer[2][ARRAY_Y_SIZE][ARRAY_X_SIZE * 2 / 8];
static uint8_t swap_request;
static uint8_t active_buffer;

void led_array_swap_buffer()
{
	swap_request = 1;
}

void led_array_backbuffer_bit_set(uint8_t x, uint8_t y, uint8_t color)
{
	uint8_t x_offset = x * 2 / 8;
	uint8_t x_shift = (6 - ((x * 2) % 8));

	uint8_t val = buffer[active_buffer^1][y][x_offset];
	val &= ~(3 << x_shift);
	val |= (color & 3) << x_shift;

	buffer[active_buffer^1][y][x_offset] = val;
}

/**
 * led_array_frontbuffer_bit_get() - get color of pixel
 */
static __attribute__((always_inline)) uint8_t led_array_frontbuffer_bit_get(uint8_t x, uint8_t y)
{
	uint8_t x_offset = x * 2 / 8;
	uint8_t x_shift = (6 - ((x * 2) % 8));

	return buffer[active_buffer][y][x_offset] >> x_shift;
}

static uint8_t greyscale_counter;

/**
 * led_array_output_bit() - set bit in shift register
 * @color:	color value
 *
 * output is low active
 */
static __attribute__((always_inline)) void led_array_output_bit(uint8_t color)
{
	if (greyscale_counter < color)
		OUTPUT_CLEAR(OUTPUT_PORT_BITS, OUTPUT_PIN_BITS);
	else
		OUTPUT_SET(OUTPUT_PORT_BITS, OUTPUT_PIN_BITS);
}

/**
 * led_array_output_line() - change actual line
 * @line:	0 ... ARRAY_Y_SIZES - 1
 */
static __attribute__((always_inline)) void led_array_output_line(uint8_t line)
{
	OUTPUT_PORT_LINE = (OUTPUT_PORT_LINE & ~OUTPUT_MASK_LINE) |
		(line & OUTPUT_MASK_LINE);
}

void led_array_draw()
{
	uint8_t y, x, j;
	uint8_t color;

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
			color = led_array_frontbuffer_bit_get(x, y);
			led_array_output_bit(color);

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

	/* change buffers */
	if (swap_request) {
		swap_request = 0;
		active_buffer ^= 1;
	}

	ASM_DELAY(j, 255);
	greyscale_counter++;

	/* TODO: check if %4 is more efficient */
	if (greyscale_counter == 16)
		greyscale_counter = 0;

	/* disable output; fail? is disabled already? */
	OUTPUT_ENABLE(0);

	//TIMSK0 = 1;
}
