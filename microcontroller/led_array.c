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
	/* clear colour of pixel */
	val &= ~(3 << x_shift);
	/* set color of pixel */
	val |= (color & 3) << x_shift;
	/* write value back */
	buffer[active_buffer^1][y][x_offset] = val;
}

/**
 * led_array_frontbuffer_bit_get() - get color of pixel
 */
static __attribute__((always_inline)) uint8_t led_array_frontbuffer_bit_get(uint8_t x, uint8_t y)
{
	uint8_t x_offset = x >> 2; /* was: (x * 2) / 8 */
//	uint8_t x_shift = (6 - ((x * 2) % 8));
	uint8_t x_shift = 2;

	uint8_t val = buffer[active_buffer][y][x_offset];
	val >>= x_shift;

	return val & 3;
}

static __attribute__((always_inline)) void led_array_output_enable(uint8_t on)
{
	if (on)
		OUTPUT_CLEAR(OUTPUT_PORT_ENABLE, OUTPUT_PIN_ENABLE);
	else
		OUTPUT_SET(OUTPUT_PORT_ENABLE, OUTPUT_PIN_ENABLE);
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
	uint8_t y, x;
	uint8_t color;

	/* enable output */
	led_array_output_enable(1);

	for (y = 0; y < ARRAY_Y_SIZE; y++) {
		//shift = 1 << y;
		for (x = 0; x < ARRAY_X_SIZE; x++) {
			/* clock low */
			OUTPUT_CLOCK(0);

			color = led_array_frontbuffer_bit_get(x, y);
			led_array_output_bit(color);

			/* clock high */
			OUTPUT_CLOCK(1);
		}
		/* change line; if not done here old line is still bright a bit */
		led_array_output_line(y);

		/* let shift registers output */
		OUTPUT_STORE_TOGGLE();
		OUTPUT_STORE_TOGGLE();
	}

	/* change buffers */
	if (swap_request) {
		swap_request = 0;
		active_buffer ^= 1;
	}

	greyscale_counter++;

	/* TODO: check if %4 is more efficient */
	if (greyscale_counter == 16)
		greyscale_counter = 0;

	led_array_output_enable(0);
}
