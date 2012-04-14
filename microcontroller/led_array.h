#ifndef LED_ARRAY_H
#define LED_ARRAY_H

#include <avr/io.h>

/**
 * led_array_swap_buffer() - switch backbuffer to frontend buffer
 */
extern void led_array_swap_buffer();

/**
 * led_array_backbuffer_bit_set() - set a LED in backbuffer
 * @x:		x coordinate
 * @y:		y coordinate
 * @color:	color value
 */
extern void led_array_backbuffer_bit_set(uint8_t x, uint8_t y, uint8_t color);

/**
 * led_array_draw() - draw led array
 *
 * was: ISR (TIMER0_OVF_vect)
 */
void led_array_draw();

#endif /* LED_ARRAY_H */
