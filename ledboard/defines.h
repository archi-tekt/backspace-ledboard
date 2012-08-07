#ifndef DEFINES_H
#define	DEFINES_H

#define BAUDRATE 115200
#define ACK_BYTE 0xFE

/* set CPU type; defined in makefile but helpful for autocomplete */
#ifndef __AVR_ATmega168__
#define __AVR_ATmega168__
#endif

/* setup clock; e.g. for delay funcs needed */
#define F_CPU   18432000UL

/* general defines for easy PORT access */
#define OUTPUT_TOGGLE(port,pin) ((port = port ^ (1 << pin)))
#define OUTPUT_SET(port,pin) ((port |= 1 << pin))
#define OUTPUT_CLEAR(port,pin) ((port &= ~(1 << pin)) )
#define INPUT_GET(port,pin) (((port >> pin) & 1))

#define ASM_DELAY(var, count) {for(var = 0; var < count; var++ ) asm volatile("nop");}

/* 
 * definition of output signals
 * C name - PCB name - description
 * ENABLE - HE - global signal to en- or disable output to LEDs
 * LINE - A, B, C, D - multiplexing of the 16 lines
 * CLOCK - CLK/SCK/SLK - shift clock for a row of LEDs
 * BITS - R0 - shift bits for a row of LEDs, low active
 * STORE - STR - TODO
 * TODO - RCK - reset, clears a complete row immediately -> might bring multiplexing timing issues
 */

/* redefine PINs and PORTs for understandable access */
#define OUTPUT_PORT_ENABLE      PORTC
#define OUTPUT_PORT_LINE        PORTC
#define OUTPUT_PORT_CLOCK       PORTB
#define OUTPUT_PORT_BITS        PORTD
#define OUTPUT_PORT_STORE       PORTC

#define OUTPUT_PIN_ENABLE       PINC4
#define OUTPUT_PIN_CLOCK        PINB1
#define OUTPUT_PIN_BITS         PIND4
#define OUTPUT_PIN_STORE        PINC5
#define OUTPUT_MASK_LINE        0x0F

#endif	/* DEFINES_H */
