#ifndef LED_ARRAY_H
#define LED_ARRAY_H

#include <avr/io.h>

/**
 * led_array_swap_buffer() - switch backbuffer to frontend buffer
 */
extern void led_array_swap_buffer();

/**
 * led_array_backbuffer_free() - check if backbuffer is free for a new frame
 */
uint8_t led_array_backbuffer_free();

/**
 * led_array_backbuffer_stream_rewind() - set position for stream_write to 0, 0
 */
extern void led_array_backbuffer_stream_rewind();

/**
 * led_array_backbuffer_stream_write() - write a byte to backbuffer
 */
extern void led_array_backbuffer_stream_write(uint8_t);

/**
 * led_array_draw() - draw led array
 *
 * was: ISR (TIMER0_OVF_vect)
 */
void led_array_draw();

#endif /* LED_ARRAY_H */
