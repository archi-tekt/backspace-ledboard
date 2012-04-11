#define ARRAY_X_SIZE	96
#define ARRAY_Y_SIZE	16

#include <avr/io.h>
#include <avr/interrupt.h>

#define toggle(port,pin) ((port = port ^ (1 << pin)))
#define set(port,pin) ((port |= 1 << pin))
#define clear(port,pin) ((port &= ~(1 << pin)) )
#define get(port,pin) (((port >> pin) & 1))

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
        unsigned char i = 0, t, j;
        unsigned short shift;
        //cli();

#if 0
        TIMSK0 = 0;
        //sei();

        TCNT0 = 0xd5;
#endif
        /* disable output */
        clear ( PORTC, 4 );

        /* set line 0 */
        PORTC = (PORTC & 0xF0);

        for ( i = 0; i < ARRAY_Y_SIZE; i++ )
        {
                shift = 1 << i;
                for ( t = 0; t < ARRAY_X_SIZE; t++ )
                {
                        /* clock low */
                        clear ( PORTB, 1 );
#if 0
                        /* update bits */
                        if ( frame_buffer[t] & shift )
                                clear ( PORTD, 4 );
                        else
                                set ( PORTD, 4 );
#endif
			clear ( PORTD, 4 );

                        /* clock high */
                        set ( PORTB, 1 );

                }

                /* enable output */
                set ( PORTC, 4 );

                /* let the leds bright for some time */
                for ( t = 0; t < 10; t++ )
                        asm volatile ( "nop" );

                /* save? str */
                toggle( PORTC, 5 );
                toggle( PORTC, 5 );

		/* change line */
                PORTC = (PORTC & 0xF0) | i;

                /* disable output */
                clear ( PORTC, 4 );

		for ( t = 0; t < 10; t++ )
			asm volatile ( "nop" );
                for ( t = 0; t < 10; t++ ) {
			asm volatile ( "nop" );
		}
      }

#if 0
        /* change buffers */
        if ( update_request )
        {
                update_request = 0;
                for ( i = 0; i < 96; i++ )
                        frame_buffer[i] = frame[i];
        }
#endif
        for ( t = 0; t < 255; t++ ) {
                asm volatile ( "nop" );

	}

	/* disable output */
        set ( PORTC, 4 );

//        TIMSK0 = 1;
}
