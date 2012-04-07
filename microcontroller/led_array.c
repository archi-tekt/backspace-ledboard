#define ARRAY_X_SIZE	96
#define ARRAY_Y_SIZE	16

#include <avr/io.h>
#include <avr/interrupt.h>

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

ISR(TIMER0_OVF_vect)
{
	uint8_t x, y;

	for (y = 0; y < 16; y++) {
		for (x = 0; x < 96; x++) {

		}
	}


}
