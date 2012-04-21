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

static uint8_t stream_x;
static uint8_t stream_y;

void led_array_backbuffer_stream_rewind()
{
	stream_x = 0;
	stream_y = 0;
}

void led_array_backbuffer_stream_write(uint8_t data)
{
	buffer[active_buffer^1][stream_y][stream_x] = data;

	stream_x++;

	if (stream_x == ARRAY_X_SIZE * 2 / 8) {
		stream_x = 0;
		stream_y++;
		if (stream_y == ARRAY_Y_SIZE)
			stream_y = 0;
	}
}

void led_array_backbuffer_bit_set(uint8_t x, uint8_t y, uint8_t color)
{
	uint8_t x_offset = x * 2 / 8;
	uint8_t x_shift = (x * 2) % 8;

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
static __attribute__((always_inline)) uint8_t led_array_frontbuffer_bit_get(uint8_t x, uint8_t *line_data)
{
	uint8_t x_offset = x >> 2; /* was: (x * 2) / 8 */
	uint8_t x_shift = (6 - ((x *2) % 8));
//	uint8_t x_shift = 2;

	uint8_t val = line_data[x_offset];
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

static const uint8_t color_mapping[] = {0,1,3,7};

void led_array_draw()
{
	uint8_t y, x;
	uint8_t color;
	uint8_t *current_line;
	uint8_t x_shift;
	uint8_t tmp;
	uint16_t wait;

	for (y = 0; y < ARRAY_Y_SIZE; y++) {
		/* less array lookups on pixel shift */
		current_line = buffer[active_buffer][y];

		x_shift = 0;
		tmp = *current_line;

		for (x = 0; x < ARRAY_X_SIZE; x++) {
			/* clock low */
			OUTPUT_CLOCK(0);

			color = color_mapping[(tmp) & 3];
			tmp >>= 2;

			led_array_output_bit(color);

			if ( x_shift == 3 ) {
				current_line++;
				tmp = *current_line;
				x_shift = 0;
			} else
				x_shift++;

			/* clock high; shift bits */
			OUTPUT_CLOCK(1);
		}

		led_array_output_enable(0);

		/* minimum delay so other lines will not bright */
		ASM_DELAY(tmp, 13);

		/* change line */
		led_array_output_line(y);

		/* let shift registers output */
		OUTPUT_STORE_TOGGLE();
		OUTPUT_STORE_TOGGLE();

		led_array_output_enable(1);
	}

	/* change buffers */
	if (swap_request) {
		swap_request = 0;
		active_buffer ^= 1;
	}

	greyscale_counter++;

	if (greyscale_counter == 8)
		greyscale_counter = 0;

	/* last line delay, approx same time as 96 led shifts */
	ASM_DELAY(wait,350);

	led_array_output_enable(0); 
}
